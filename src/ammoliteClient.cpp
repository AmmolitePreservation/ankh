#include "ammoliteClient.h"

#include "fatal.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {

struct ReplyError {
  QString message;
};

QJsonValue requiredValue(const QJsonObject& object, const char* key)
{
  const QJsonValue value = object.value(QLatin1String(key));
  if (value.isUndefined()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is missing").arg(QLatin1String(key))};
  }
  return value;
}

QString requiredString(const QJsonObject& object, const char* key)
{
  const QJsonValue value = requiredValue(object, key);
  if (!value.isString()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not text").arg(QLatin1String(key))};
  }
  return value.toString();
}

int requiredInt(const QJsonObject& object, const char* key)
{
  const QJsonValue value = requiredValue(object, key);
  if (!value.isDouble() || value.toDouble() != value.toInteger()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not a whole number").arg(QLatin1String(key))};
  }
  return static_cast<int>(value.toInteger());
}

QString optionalString(const QJsonObject& object, const char* key)
{
  const QJsonValue value = requiredValue(object, key);
  if (value.isNull()) {
    return QString();
  }
  if (!value.isString()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not text").arg(QLatin1String(key))};
  }
  return value.toString();
}

std::optional<int> optionalInt(const QJsonObject& object, const char* key)
{
  if (requiredValue(object, key).isNull()) {
    return std::nullopt;
  }
  return requiredInt(object, key);
}

QJsonArray requiredArray(const QJsonObject& object, const char* key)
{
  const QJsonValue value = requiredValue(object, key);
  if (!value.isArray()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not a list").arg(QLatin1String(key))};
  }
  return value.toArray();
}

QJsonObject objectAt(const QJsonValue& value)
{
  if (!value.isObject()) {
    throw ReplyError{QStringLiteral("a list entry is not an object")};
  }
  return value.toObject();
}

SignedInUser parseUser(const QJsonObject& object)
{
  return {requiredInt(object, "userID"), requiredString(object, "username"), requiredString(object, "role")};
}

qint64 requiredCount(const QJsonObject& object, const char* key)
{
  const QJsonValue value = requiredValue(object, key);
  if (!value.isDouble() || value.toDouble() != value.toInteger() || value.toInteger() < 0) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not a whole number").arg(QLatin1String(key))};
  }
  return value.toInteger();
}

QUrl requiredUrl(const QJsonObject& object, const char* key)
{
  const QUrl url(requiredString(object, key), QUrl::StrictMode);
  if (!url.isValid() || url.scheme().isEmpty()) {
    throw ReplyError{QStringLiteral("the field \"%1\" is not an address").arg(QLatin1String(key))};
  }
  return url;
}

World parseWorld(const QJsonObject& object)
{
  const QColor color(requiredString(object, "color"));
  if (!color.isValid()) {
    throw ReplyError{QStringLiteral("the color of a world is not a color")};
  }
  return {requiredInt(object, "serverID"), requiredString(object, "name"), requiredString(object, "game"),
    requiredString(object, "variant"), requiredString(object, "version"), color, requiredInt(object, "eraStart"),
    requiredInt(object, "eraEnd"), optionalString(object, "status"), optionalInt(object, "accountDomainID"),
    optionalString(object, "accountDomain")};
}

Edition parseEdition(const QJsonObject& object)
{
  return {requiredString(object, "game"), requiredString(object, "variant"), requiredString(object, "edition")};
}

PackageVersion parsePackageVersion(const QJsonObject& object)
{
  return {requiredString(object, "version"), requiredString(object, "fileName"), requiredCount(object, "byteSize"),
    requiredString(object, "sha256"), requiredString(object, "publishedAt"),
    static_cast<int>(requiredCount(object, "fileCount")), requiredCount(object, "extractedBytes"),
    requiredUrl(object, "apiURL")};
}

Package parsePackage(const QJsonObject& object)
{
  static const QStringList roles = {QStringLiteral("client"), QStringLiteral("requiredTool"), QStringLiteral("optionalTool")};
  Package package{requiredString(object, "name"), requiredString(object, "label"), requiredString(object, "role"), {}, std::nullopt,
    requiredString(object, "description"), std::nullopt};
  if (!roles.contains(package.role)) {
    throw ReplyError{QStringLiteral("the role of the package %1 is unknown: %2").arg(package.name, package.role)};
  }
  for (const QJsonValue& value : requiredArray(object, "editions")) {
    package.editions.append(parseEdition(objectAt(value)));
  }
  const QJsonValue latest = requiredValue(object, "latest");
  if (!latest.isNull()) {
    package.latest = parsePackageVersion(objectAt(latest));
  }
  const QJsonValue screenshot = requiredValue(object, "screenshot");
  if (!screenshot.isNull()) {
    const QJsonObject described = objectAt(screenshot);
    package.screenshot = PackageScreenshot{requiredUrl(described, "url"), requiredString(described, "sha256"),
      requiredString(described, "contentType")};
  }
  return package;
}

AnkhRelease parseRelease(const QJsonObject& object)
{
  AnkhRelease release{requiredString(object, "version"), requiredString(object, "tag"), requiredString(object, "notes"),
    requiredUrl(object, "page"), requiredString(object, "publishedAt"), {}};
  const QJsonValue assets = requiredValue(object, "assets");
  if (!assets.isObject()) {
    throw ReplyError{QStringLiteral("the field \"assets\" is not an object")};
  }
  const QJsonObject assetObject = assets.toObject();
  for (auto entry = assetObject.constBegin(); entry != assetObject.constEnd(); ++entry) {
    release.assets.insert(entry.key(), requiredUrl(assetObject, entry.key().toUtf8().constData()));
  }
  for (const QJsonValue& value : requiredArray(object, "releases")) {
    const QJsonObject listed = objectAt(value);
    release.history.append({requiredString(listed, "version"), requiredString(listed, "notes"), requiredString(listed, "publishedAt")});
  }
  return release;
}

Manifest parseManifest(const QJsonObject& object)
{
  Manifest manifest{std::nullopt, QString(), false, {}, {}, {}};
  const QJsonObject ankh = objectAt(requiredValue(object, "ankh"));
  if (ankh.contains(QLatin1String("error"))) {
    manifest.ankhProblem = requiredString(ankh, "error");
  } else {
    manifest.ankh = parseRelease(ankh);
  }
  if (object.contains(QLatin1String("worlds"))) {
    manifest.hasCollection = true;
    for (const QJsonValue& value : requiredArray(object, "worlds")) {
      manifest.worlds.append(parseWorld(objectAt(value)));
    }
    for (const QJsonValue& value : requiredArray(object, "packages")) {
      manifest.packages.append(parsePackage(objectAt(value)));
    }
    for (const QJsonValue& value : requiredArray(object, "attachedGames")) {
      const QJsonObject attached = objectAt(value);
      const QColor color(requiredString(attached, "color"));
      const QString executable = requiredString(attached, "executable");
      if (!color.isValid()) {
        throw ReplyError{QStringLiteral("the color of an attached game is not a color")};
      }
      if (executable.isEmpty() || executable.startsWith(QLatin1Char('/')) || executable.contains(QLatin1Char('\\'))
          || executable.contains(QLatin1Char(':')) || executable.split(QLatin1Char('/')).contains(QStringLiteral(".."))) {
        throw ReplyError{QStringLiteral("the executable of an attached game is not a path inside its folder: %1").arg(executable)};
      }
      manifest.attachedGames.append({requiredString(attached, "game"), requiredString(attached, "variant"),
        requiredString(attached, "edition"), color, executable});
    }
  }
  return manifest;
}

AccountDomain parseAccountDomain(const QJsonObject& object)
{
  AccountDomain domain{requiredInt(object, "accountDomainID"), requiredString(object, "name"), {}, {}};
  for (const QJsonValue& capability : requiredArray(object, "capabilities")) {
    domain.capabilities.append(capability.toString());
  }
  for (const QJsonValue& loginValue : requiredArray(object, "logins")) {
    const QJsonObject login = objectAt(loginValue);
    domain.logins.append({requiredInt(login, "gameAccountID"), requiredString(login, "gameUsername")});
  }
  return domain;
}

}

QUrl validatedServiceUrl(const QString& text)
{
  const QUrl url(text, QUrl::StrictMode);
  const bool isLoopback = url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String("localhost");
  const bool isAllowed = url.scheme() == QLatin1String("https") || (url.scheme() == QLatin1String("http") && isLoopback);
  if (!url.isValid() || !isAllowed || url.host().isEmpty() || !url.path().isEmpty() || url.hasQuery() || url.hasFragment() || !url.userInfo().isEmpty()) {
    fatal(QStringLiteral("The Ammolite address must look like https://host:port, and may use http only for localhost: %1").arg(text));
  }
  return url;
}

AmmoliteClient::AmmoliteClient(const QUrl& baseUrl, const QString& appVersion, QObject* parent)
  : QObject(parent), base(baseUrl), appVersion(appVersion), network(new QNetworkAccessManager(this))
{
}

QUrl AmmoliteClient::baseUrl() const
{
  return base;
}

QString AmmoliteClient::token() const
{
  return apiToken;
}

void AmmoliteClient::setToken(const QString& newToken)
{
  apiToken = newToken;
}

QNetworkRequest AmmoliteClient::requestFor(const QString& path, bool isAuthorized) const
{
  QUrl url = base;
  url.setPath(path);
  QNetworkRequest request(url);
  request.setRawHeader("Accept", "application/json");
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(appVersion));
  request.setTransferTimeout(20000);
  if (isAuthorized) {
    request.setRawHeader("Authorization", "Bearer " + apiToken.toUtf8());
  }
  return request;
}

void AmmoliteClient::finish(QNetworkReply* reply, bool isAuthorized,
  const std::function<void(const QJsonObject&, QNetworkReply*)>& onSuccess,
  const std::function<void(const QString&)>& onFailure)
{
  connect(reply, &QNetworkReply::finished, this, [this, reply, isAuthorized, onSuccess, onFailure] {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status == 304) {
      onSuccess(QJsonObject(), reply);
      return;
    }
    const QByteArray payload = reply->readAll();
    const QJsonDocument document = QJsonDocument::fromJson(payload);
    if (reply->error() != QNetworkReply::NoError) {
      if (status == 401 && isAuthorized) {
        emit sessionExpired();
        return;
      }
      const QString reason = document.isObject() ? document.object().value(QStringLiteral("error")).toString() : QString();
      onFailure(reason.isEmpty() ? reply->errorString() : reason);
      return;
    }
    if (!document.isObject()) {
      onFailure(QStringLiteral("The server's answer was not understood."));
      return;
    }
    try {
      onSuccess(document.object(), reply);
    } catch (const ReplyError& error) {
      onFailure(QStringLiteral("The server's answer was not understood: %1.").arg(error.message));
    }
  });
}

void AmmoliteClient::send(const QByteArray& method, const QString& path, const QJsonObject& body, bool isAuthorized,
  const std::function<void(const QJsonObject&)>& onSuccess, const std::function<void(const QString&)>& onFailure)
{
  QNetworkRequest request = requestFor(path, isAuthorized);
  QNetworkReply* reply = nullptr;
  if (method == "POST") {
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    reply = network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
  } else if (method == "DELETE") {
    reply = network->deleteResource(request);
  } else {
    reply = network->get(request);
  }
  finish(reply, isAuthorized, [onSuccess](const QJsonObject& answer, QNetworkReply*) { onSuccess(answer); }, onFailure);
}

void AmmoliteClient::signIn(const QString& username, const QString& password, const QString& tokenLabel)
{
  startSignIn(username, password, tokenLabel, [this](const QString& message) { emit signInFailed(message); });
}

void AmmoliteClient::registerAccount(const QString& inviteCode, const QString& username, const QString& password,
  const QString& confirmation, const QString& tokenLabel)
{
  QJsonObject body;
  body.insert(QStringLiteral("inviteCode"), inviteCode);
  body.insert(QStringLiteral("username"), username);
  body.insert(QStringLiteral("password"), password);
  body.insert(QStringLiteral("passwordConfirmation"), confirmation);
  send("POST", QStringLiteral("/api/v1/register"), body, false,
    [this, username, password, tokenLabel](const QJsonObject&) {
      startSignIn(username, password, tokenLabel, [this](const QString& message) {
        emit registerFailed(QStringLiteral("The account was created, but signing in failed: %1").arg(message));
      });
    },
    [this](const QString& message) { emit registerFailed(message); });
}

void AmmoliteClient::startSignIn(const QString& username, const QString& password, const QString& tokenLabel,
  const std::function<void(const QString&)>& onFailure)
{
  QJsonObject body;
  body.insert(QStringLiteral("username"), username);
  body.insert(QStringLiteral("password"), password);
  body.insert(QStringLiteral("label"), tokenLabel);
  send("POST", QStringLiteral("/api/v1/auth/token"), body, false,
    [this](const QJsonObject& answer) {
      const SignedInUser user = parseUser(requiredValue(answer, "user").toObject());
      const QString newToken = requiredString(answer, "token");
      apiToken = newToken;
      emit signedIn(user, newToken);
    },
    onFailure);
}

void AmmoliteClient::signOut()
{
  send("DELETE", QStringLiteral("/api/v1/auth/token"), QJsonObject(), true,
    [this](const QJsonObject&) {
      apiToken.clear();
      emit signedOut(QString());
    },
    [this](const QString& message) {
      apiToken.clear();
      emit signedOut(QStringLiteral("The sign-in could not be revoked on the server: %1").arg(message));
    });
}

void AmmoliteClient::loadMe()
{
  send("GET", QStringLiteral("/api/v1/me"), QJsonObject(), true,
    [this](const QJsonObject& answer) { emit meLoaded(parseUser(answer)); },
    [this](const QString& message) { emit requestFailed(QStringLiteral("account"), message); });
}

void AmmoliteClient::loadManifest(const QByteArray& etag)
{
  const bool isAuthorized = !apiToken.isEmpty();
  QNetworkRequest request = requestFor(QStringLiteral("/api/v1/manifest"), isAuthorized);
  if (!etag.isEmpty()) {
    request.setRawHeader("If-None-Match", etag);
  }
  finish(network->get(request), isAuthorized,
    [this](const QJsonObject& answer, QNetworkReply* reply) {
      if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 304) {
        emit manifestUnchanged();
        return;
      }
      emit manifestLoaded(parseManifest(answer), reply->rawHeader("ETag"));
    },
    [this](const QString& message) { emit requestFailed(QStringLiteral("manifest"), message); });
}

void AmmoliteClient::loadGameAccounts()
{
  send("GET", QStringLiteral("/api/v1/me/gameAccounts"), QJsonObject(), true,
    [this](const QJsonObject& answer) {
      QList<AccountDomain> domains;
      for (const QJsonValue& value : requiredArray(answer, "domains")) {
        domains.append(parseAccountDomain(objectAt(value)));
      }
      emit gameAccountsLoaded(domains);
    },
    [this](const QString& message) { emit requestFailed(QStringLiteral("game accounts"), message); });
}

// Images are cached by their SHA-256, so a screenshot that changed on the server is fetched again and an unchanged one
// never is. A cached file whose hash does not match its name is discarded.
void AmmoliteClient::loadScreenshot(const QString& packageName, const PackageScreenshot& screenshot, const QString& cacheFolder)
{
  const QString cachePath = QDir(cacheFolder).filePath(screenshot.sha256);
  QFile cached(cachePath);
  if (cached.open(QIODevice::ReadOnly)) {
    const QByteArray image = cached.readAll();
    cached.close();
    if (QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex() == screenshot.sha256.toLatin1()) {
      QMetaObject::invokeMethod(this, [this, packageName, hash = screenshot.sha256, image] {
        emit screenshotLoaded(packageName, hash, image);
      }, Qt::QueuedConnection);
      return;
    }
    QFile::remove(cachePath);
  }
  QNetworkRequest request(screenshot.url);
  request.setRawHeader("Authorization", "Bearer " + apiToken.toUtf8());
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(appVersion));
  request.setTransferTimeout(20000);
  QNetworkReply* reply = network->get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply, packageName, screenshot, cacheFolder, cachePath] {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit screenshotFailed(packageName, reply->errorString());
      return;
    }
    const QByteArray image = reply->readAll();
    if (QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex() != screenshot.sha256.toLatin1()) {
      emit screenshotFailed(packageName, QStringLiteral("The screenshot did not match the checksum Ammolite gave for it."));
      return;
    }
    QDir().mkpath(cacheFolder);
    QFile target(cachePath);
    if (target.open(QIODevice::WriteOnly)) {
      target.write(image);
    }
    emit screenshotLoaded(packageName, screenshot.sha256, image);
  });
}
