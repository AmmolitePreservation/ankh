#include "ammoliteClient.h"

#include "fatal.h"

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

World parseWorld(const QJsonObject& object)
{
  const QColor color(requiredString(object, "color"));
  if (!color.isValid()) {
    throw ReplyError{QStringLiteral("the color of a world is not a color")};
  }
  return {requiredInt(object, "serverID"), requiredString(object, "name"), requiredString(object, "game"),
    requiredString(object, "version"), color, requiredInt(object, "eraStart"), requiredInt(object, "eraEnd"),
    optionalString(object, "status"), optionalInt(object, "accountDomainID"), optionalString(object, "accountDomain")};
}

QList<DownloadGroup> parseDownloads(const QJsonArray& groups)
{
  QList<DownloadGroup> result;
  for (const QJsonValue& groupValue : groups) {
    const QJsonObject group = objectAt(groupValue);
    DownloadGroup parsed{requiredString(group, "category"), {}};
    for (const QJsonValue& packageValue : requiredArray(group, "packages")) {
      const QJsonObject package = objectAt(packageValue);
      parsed.packages.append({requiredInt(package, "packageVersionID"), requiredString(package, "label"),
        requiredString(package, "version"), requiredString(package, "fileName"),
        static_cast<qint64>(requiredValue(package, "byteSize").toInteger(-1)), requiredString(package, "sha256")});
      if (parsed.packages.last().byteSize < 0) {
        throw ReplyError{QStringLiteral("the field \"byteSize\" is not a whole number")};
      }
    }
    result.append(parsed);
  }
  return result;
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

void AmmoliteClient::setToken(const QString& newToken)
{
  token = newToken;
}

void AmmoliteClient::send(const QByteArray& method, const QString& path, const QJsonObject& body, bool isAuthorized,
  const std::function<void(const QJsonObject&)>& onSuccess, const std::function<void(const QString&)>& onFailure)
{
  QUrl url = base;
  url.setPath(path);
  QNetworkRequest request(url);
  request.setRawHeader("Accept", "application/json");
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(appVersion));
  request.setTransferTimeout(20000);
  if (isAuthorized) {
    request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
  }
  QNetworkReply* reply = nullptr;
  if (method == "POST") {
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    reply = network->post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
  } else if (method == "DELETE") {
    reply = network->deleteResource(request);
  } else {
    reply = network->get(request);
  }
  connect(reply, &QNetworkReply::finished, this, [this, reply, isAuthorized, onSuccess, onFailure] {
    reply->deleteLater();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
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
      onSuccess(document.object());
    } catch (const ReplyError& error) {
      onFailure(QStringLiteral("The server's answer was not understood: %1.").arg(error.message));
    }
  });
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
      token = newToken;
      emit signedIn(user, newToken);
    },
    onFailure);
}

void AmmoliteClient::signOut()
{
  send("DELETE", QStringLiteral("/api/v1/auth/token"), QJsonObject(), true,
    [this](const QJsonObject&) {
      token.clear();
      emit signedOut(QString());
    },
    [this](const QString& message) {
      token.clear();
      emit signedOut(QStringLiteral("The sign-in could not be revoked on the server: %1").arg(message));
    });
}

void AmmoliteClient::loadMe()
{
  send("GET", QStringLiteral("/api/v1/me"), QJsonObject(), true,
    [this](const QJsonObject& answer) { emit meLoaded(parseUser(answer)); },
    [this](const QString& message) { emit requestFailed(QStringLiteral("account"), message); });
}

void AmmoliteClient::loadWorlds()
{
  send("GET", QStringLiteral("/api/v1/worlds"), QJsonObject(), true,
    [this](const QJsonObject& answer) {
      QList<World> worlds;
      for (const QJsonValue& value : requiredArray(answer, "worlds")) {
        worlds.append(parseWorld(objectAt(value)));
      }
      emit worldsLoaded(worlds);
    },
    [this](const QString& message) { emit requestFailed(QStringLiteral("worlds"), message); });
}

void AmmoliteClient::loadWorld(int serverID)
{
  send("GET", QStringLiteral("/api/v1/worlds/%1").arg(serverID), QJsonObject(), true,
    [this, serverID](const QJsonObject& answer) {
      emit worldLoaded(serverID, parseDownloads(requiredArray(answer, "downloads")));
    },
    [this, serverID](const QString& message) { emit worldFailed(serverID, message); });
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
