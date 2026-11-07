#include "updateInstaller.h"

#include "calendarVersion.h"
#include "installInfo.h"
#include "updateSignature.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QStandardPaths>
#include <QWinEventNotifier>
#include <functional>
#include <memory>

#define NOMINMAX
#include <windows.h>

namespace {

QNetworkRequest assetRequest(const QUrl& url, const QString& version)
{
  QNetworkRequest request(url);
  request.setRawHeader("Accept", "application/octet-stream");
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(version));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setTransferTimeout(30000);
  return request;
}

QString updateFolder()
{
  return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath(QStringLiteral("ankhUpdate"));
}

}

void removeUpdateLeftovers()
{
  QDir(updateFolder()).removeRecursively();
}

UpdateInstaller::UpdateInstaller(const UpdateOffer& offer, const QString& currentVersion, QObject* parent)
  : QObject(parent), offer(offer), currentVersion(currentVersion), network(new QNetworkAccessManager(this))
{
}

void UpdateInstaller::start()
{
  if (installScope() == InstallScope::buildFolder) {
    emit failed(QStringLiteral("This copy of Ankh was not set up by the installer, so it cannot update itself."));
    return;
  }
  workFolder = updateFolder();
  QDir(workFolder).removeRecursively();
  if (!QDir().mkpath(workFolder)) {
    emit failed(QStringLiteral("Could not create a folder for the update in %1.").arg(workFolder));
    return;
  }
  emit statusChanged(QStringLiteral("Downloading the update details..."));
  fetch(QStringLiteral("manifest.json"), [this](const QByteArray& manifest) {
    fetch(QStringLiteral("manifest.json.sig"), [this, manifest](const QByteArray& signature) {
      verifyManifest(manifest, signature);
    });
  });
}

void UpdateInstaller::fetch(const QString& assetName, std::function<void(const QByteArray&)> onFinished)
{
  const auto found = offer.assets.constFind(assetName);
  if (found == offer.assets.constEnd()) {
    emit failed(QStringLiteral("The release has no %1.").arg(assetName));
    return;
  }
  QNetworkReply* reply = network->get(assetRequest(found.value(), currentVersion));
  connect(reply, &QNetworkReply::finished, this, [this, reply, assetName, onFinished] {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit failed(QStringLiteral("Could not download %1: %2").arg(assetName, reply->errorString()));
      return;
    }
    onFinished(reply->readAll());
  });
}

void UpdateInstaller::verifyManifest(const QByteArray& manifest, const QByteArray& signature)
{
  emit statusChanged(QStringLiteral("Checking the signature..."));
  if (!verifyManifestSignature(manifest, signature)) {
    emit failed(QStringLiteral("The update's signature is not valid, so it was not installed."));
    return;
  }
  const QJsonObject object = QJsonDocument::fromJson(manifest).object();
  const QString version = object.value(QStringLiteral("version")).toString();
  const auto offered = parseCalendarVersion(version);
  const auto current = parseCalendarVersion(currentVersion);
  if (!offered || version != offer.version) {
    emit failed(QStringLiteral("The signed manifest names a different version than the release, so it was not installed."));
    return;
  }
  if (*offered <= *current) {
    emit failed(QStringLiteral("The signed update is not newer than this version, so it was not installed."));
    return;
  }
  const QString installer = object.value(QStringLiteral("installer")).toString();
  const qint64 size = object.value(QStringLiteral("size")).toVariant().toLongLong();
  const QString sha256 = object.value(QStringLiteral("sha256")).toString().toLower();
  if (installer.isEmpty() || QFileInfo(installer).fileName() != installer || size <= 0 || sha256.size() != 64) {
    emit failed(QStringLiteral("The signed manifest is incomplete, so the update was not installed."));
    return;
  }
  downloadInstaller(installer, size, sha256);
}

void UpdateInstaller::downloadInstaller(const QString& assetName, qint64 size, const QString& sha256)
{
  const auto found = offer.assets.constFind(assetName);
  if (found == offer.assets.constEnd()) {
    emit failed(QStringLiteral("The release has no %1.").arg(assetName));
    return;
  }
  const QString path = QDir(workFolder).filePath(assetName);
  auto file = std::make_shared<QFile>(path);
  if (!file->open(QIODevice::WriteOnly)) {
    emit failed(QStringLiteral("Could not write %1.").arg(path));
    return;
  }
  auto hash = std::make_shared<QCryptographicHash>(QCryptographicHash::Sha256);
  emit statusChanged(QStringLiteral("Downloading Ankh %1...").arg(offer.version));
  QNetworkReply* reply = network->get(assetRequest(found.value(), currentVersion));
  connect(reply, &QNetworkReply::readyRead, this, [reply, file, hash] {
    const QByteArray chunk = reply->readAll();
    file->write(chunk);
    hash->addData(chunk);
  });
  connect(reply, &QNetworkReply::downloadProgress, this, &UpdateInstaller::progressChanged);
  connect(reply, &QNetworkReply::finished, this, [this, reply, file, hash, path, size, sha256, assetName] {
    reply->deleteLater();
    const QByteArray rest = reply->readAll();
    file->write(rest);
    hash->addData(rest);
    file->close();
    if (reply->error() != QNetworkReply::NoError) {
      QFile::remove(path);
      emit failed(QStringLiteral("Could not download %1: %2").arg(assetName, reply->errorString()));
      return;
    }
    emit statusChanged(QStringLiteral("Checking the download..."));
    if (QFileInfo(path).size() != size || QString::fromLatin1(hash->result().toHex()) != sha256) {
      QFile::remove(path);
      emit failed(QStringLiteral("The download does not match the signed manifest, so it was deleted and not installed."));
      return;
    }
    launchInstaller(path);
  });
}

void UpdateInstaller::launchInstaller(const QString& installerPath)
{
  const InstallScope scope = installScope();
  const QStringList parts = {
    QStringLiteral("/VERYSILENT"),
    QStringLiteral("/SUPPRESSMSGBOXES"),
    QStringLiteral("/NORESTART"),
    QStringLiteral("/CLOSEAPPLICATIONS"),
    QStringLiteral("/RELAUNCH=1"),
    scope == InstallScope::allUsers ? QStringLiteral("/ALLUSERS") : QStringLiteral("/CURRENTUSER"),
    QStringLiteral("/DIR=\"%1\"").arg(installFolder()),
    QStringLiteral("/LOG=\"%1\"").arg(QDir::toNativeSeparators(QDir(workFolder).filePath(QStringLiteral("install.log")))),
  };
  QProcess process;
  process.setProgram(QDir::toNativeSeparators(installerPath));
  process.setNativeArguments(parts.join(QLatin1Char(' ')));
  qint64 processID = 0;
  if (!process.startDetached(&processID)) {
    emit failed(QStringLiteral("Windows would not start the installer."));
    return;
  }
  emit statusChanged(scope == InstallScope::allUsers
      ? QStringLiteral("Waiting for Windows permission, then installing. Ankh will restart.")
      : QStringLiteral("Installing. Ankh will restart."));
  emit installerStarted();
  HANDLE handle = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, static_cast<DWORD>(processID));
  if (handle == nullptr) {
    emit failed(QStringLiteral("Could not follow the installer. Check whether Ankh restarts."));
    return;
  }
  processWatcher = new QWinEventNotifier(handle, this);
  connect(processWatcher, &QWinEventNotifier::activated, this, [this, handle] {
    DWORD code = 0;
    GetExitCodeProcess(handle, &code);
    CloseHandle(handle);
    processWatcher->setEnabled(false);
    emit failed(code == 0
        ? QStringLiteral("The installer finished but Ankh did not restart. Close Ankh and open it again.")
        : QStringLiteral("The installer stopped before finishing (code %1), so Ankh was not updated.").arg(code));
  });
}
