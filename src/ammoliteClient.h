#pragma once

#include <QColor>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <functional>
#include <optional>

class QJsonObject;
class QNetworkAccessManager;

struct SignedInUser {
  int userID;
  QString username;
  QString role;
};

struct World {
  int serverID;
  QString name;
  QString game;
  QString version;
  QColor color;
  int eraStart;
  int eraEnd;
  QString status;
  std::optional<int> accountDomainID;
  QString accountDomain;
};

struct Download {
  int packageVersionID;
  QString label;
  QString version;
  QString fileName;
  qint64 byteSize;
  QString sha256;
};

struct DownloadGroup {
  QString category;
  QList<Download> packages;
};

struct GameLogin {
  int gameAccountID;
  QString gameUsername;
};

struct AccountDomain {
  int accountDomainID;
  QString name;
  QStringList capabilities;
  QList<GameLogin> logins;
};

QUrl validatedServiceUrl(const QString& text);

class AmmoliteClient : public QObject {
  Q_OBJECT

public:
  AmmoliteClient(const QUrl& baseUrl, const QString& appVersion, QObject* parent = nullptr);

  QUrl baseUrl() const;
  void setToken(const QString& token);
  void signIn(const QString& username, const QString& password, const QString& tokenLabel);
  void registerAccount(const QString& inviteCode, const QString& username, const QString& password,
    const QString& confirmation, const QString& tokenLabel);
  void signOut();
  void loadMe();
  void loadWorlds();
  void loadWorld(int serverID);
  void loadGameAccounts();

signals:
  void signedIn(const SignedInUser& user, const QString& token);
  void signInFailed(const QString& message);
  void registerFailed(const QString& message);
  void signedOut(const QString& warning);
  void meLoaded(const SignedInUser& user);
  void worldsLoaded(const QList<World>& worlds);
  void worldLoaded(int serverID, const QList<DownloadGroup>& downloads);
  void worldFailed(int serverID, const QString& message);
  void gameAccountsLoaded(const QList<AccountDomain>& domains);
  void sessionExpired();
  void requestFailed(const QString& what, const QString& message);

private:
  void startSignIn(const QString& username, const QString& password, const QString& tokenLabel,
    const std::function<void(const QString&)>& onFailure);
  void send(const QByteArray& method, const QString& path, const QJsonObject& body, bool isAuthorized,
    const std::function<void(const QJsonObject&)>& onSuccess, const std::function<void(const QString&)>& onFailure);

  QUrl base;
  QString appVersion;
  QString token;
  QNetworkAccessManager* network;
};
