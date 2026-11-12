#pragma once

#include <QColor>
#include <QList>
#include <QMap>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <functional>
#include <optional>

class QJsonObject;
class QNetworkAccessManager;
class QNetworkReply;

struct SignedInUser {
  int userID;
  QString username;
  QString role;
};

struct World {
  int serverID;
  QString name;
  QString game;
  QString variant;
  QString version;
  QColor color;
  int eraStart;
  int eraEnd;
  QString status;
  std::optional<int> accountDomainID;
  QString accountDomain;
};

struct Edition {
  QString game;
  QString variant;
  QString label;
};

struct PackageVersion {
  QString version;
  QString fileName;
  qint64 byteSize;
  QString sha256;
  QString publishedAt;
  int fileCount;
  qint64 extractedBytes;
  QUrl url;
};

struct PackageScreenshot {
  QUrl url;
  QString sha256;
  QString contentType;
};

struct Package {
  QString name;
  QString label;
  QString role;
  QList<Edition> editions;
  std::optional<PackageVersion> latest;
  QString description;
  std::optional<PackageScreenshot> screenshot;
  // What a plugin is listed under, one of pluginCategories; empty for a client.
  QString category;
};

// The categories of plugin packages, in the order the Plugins tab lists them.
inline const QStringList pluginCategories{QStringLiteral("Stability"), QStringLiteral("Display"), QStringLiteral("Login"),
  QStringLiteral("Portability"), QStringLiteral("Automation"), QStringLiteral("Research")};
// Plugins of this category record what a game does; they are listed only while the player asks for them in Settings.
inline const QString researchCategory = QStringLiteral("Research");

struct ReleaseNote {
  QString version;
  QString notes;
  QString publishedAt;
};

// The newest release with the files that install it; history holds every listed release, the newest included.
struct AnkhRelease {
  QString version;
  QString tag;
  QString notes;
  QUrl page;
  QString publishedAt;
  QMap<QString, QUrl> assets;
  QList<ReleaseNote> history;
};

// A game Ammolite does not host: the player has their own copy, recognized by the executable inside its folder, and
// Ankh adds the edition's plugins to it.
struct AttachedGame {
  QString game;
  QString variant;
  QString edition;
  QColor color;
  QString executable;
  // How the game is started: turbineGLS (its own launcher service, configured by the file launcherConfig in the game
  // folder) or steam (Steam starts it by steamAppID).
  QString launcher;
  QString launcherConfig;
  int steamAppID = 0;
};

struct Manifest {
  std::optional<AnkhRelease> ankh;
  QString ankhProblem;
  bool hasCollection;
  QList<World> worlds;
  QList<Package> packages;
  QList<AttachedGame> attachedGames;
  // Why the collection the server sent could not be read; empty when it was read or none was sent. The release above
  // stands either way, so an Ankh that cannot read a newer server's collection still learns of the release that can.
  QString collectionProblem;
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
  QString token() const;
  void setToken(const QString& token);
  void signIn(const QString& username, const QString& password, const QString& tokenLabel);
  void registerAccount(const QString& inviteCode, const QString& username, const QString& password,
    const QString& confirmation, const QString& tokenLabel);
  void signOut();
  void loadMe();
  void loadManifest(const QByteArray& etag);
  void loadGameAccounts();
  void loadScreenshot(const QString& packageName, const PackageScreenshot& screenshot, const QString& cacheFolder);

signals:
  void signedIn(const SignedInUser& user, const QString& token);
  void signInFailed(const QString& message);
  void registerFailed(const QString& message);
  void signedOut(const QString& warning);
  void meLoaded(const SignedInUser& user);
  void manifestLoaded(const Manifest& manifest, const QByteArray& etag);
  void manifestUnchanged();
  void gameAccountsLoaded(const QList<AccountDomain>& domains);
  void screenshotLoaded(const QString& packageName, const QString& sha256, const QByteArray& image);
  void screenshotFailed(const QString& packageName, const QString& message);
  void sessionExpired();
  void requestFailed(const QString& what, const QString& message);

private:
  void startSignIn(const QString& username, const QString& password, const QString& tokenLabel,
    const std::function<void(const QString&)>& onFailure);
  QNetworkRequest requestFor(const QString& path, bool isAuthorized) const;
  void finish(QNetworkReply* reply, bool isAuthorized,
    const std::function<void(const QJsonObject&, QNetworkReply*)>& onSuccess,
    const std::function<void(const QString&)>& onFailure);
  void send(const QByteArray& method, const QString& path, const QJsonObject& body, bool isAuthorized,
    const std::function<void(const QJsonObject&)>& onSuccess, const std::function<void(const QString&)>& onFailure);

  QUrl base;
  QString appVersion;
  QString apiToken;
  QNetworkAccessManager* network;
};
