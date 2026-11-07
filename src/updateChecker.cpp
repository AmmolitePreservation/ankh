#include "updateChecker.h"

#include "calendarVersion.h"
#include "fatal.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

UpdateChecker::UpdateChecker(const QString& currentVersion, QObject* parent)
  : QObject(parent), currentVersion(currentVersion), network(new QNetworkAccessManager(this))
{
  if (!parseCalendarVersion(currentVersion)) {
    fatal(QStringLiteral("The running version is not a calendar version: %1").arg(currentVersion));
  }
}

void UpdateChecker::check()
{
  emit checking();
  QNetworkRequest request(QUrl(QStringLiteral("https://api.github.com/repos/AmmolitePreservation/ankh/releases/latest")));
  request.setRawHeader("Accept", "application/vnd.github+json");
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(currentVersion));
  request.setTransferTimeout(15000);
  QNetworkReply* reply = network->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 404) {
      emit noReleases();
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(reply->errorString());
      return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    if (!document.isObject()) {
      emit failed(QStringLiteral("The release reply was not understood."));
      return;
    }
    const QJsonObject release = document.object();
    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    const auto latest = parseCalendarVersion(tag);
    if (!latest) {
      emit failed(QStringLiteral("The latest release tag is not a calendar version: %1").arg(tag));
      return;
    }
    const auto current = parseCalendarVersion(currentVersion);
    if (*latest <= *current) {
      emit upToDate();
      return;
    }
    UpdateOffer offer;
    offer.version = tag.startsWith(QLatin1Char('v')) ? tag.mid(1) : tag;
    offer.notes = release.value(QStringLiteral("body")).toString();
    offer.page = QUrl(release.value(QStringLiteral("html_url")).toString());
    for (const QJsonValue& asset : release.value(QStringLiteral("assets")).toArray()) {
      const QJsonObject object = asset.toObject();
      offer.assets.insert(object.value(QStringLiteral("name")).toString(), QUrl(object.value(QStringLiteral("browser_download_url")).toString()));
    }
    emit updateFound(offer);
  });
}
