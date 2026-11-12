#include "ammoliteClient.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

QByteArray fixture(const char* name)
{
  QFile file(QStringLiteral(ANKH_FIXTURE_DIR "/ammolite/") + QLatin1String(name));
  if (!file.open(QIODevice::ReadOnly)) {
    qFatal("Missing fixture %s", name);
  }
  return file.readAll();
}

class FakeServer : public QObject {
public:
  struct Recorded {
    QByteArray method;
    QByteArray path;
    QByteArray authorization;
    QByteArray ifNoneMatch;
    QByteArray body;
  };

  struct Canned {
    int status;
    QByteArray body;
    QByteArray etag;
  };

  FakeServer()
  {
    if (!server.listen(QHostAddress::LocalHost, 0)) {
      qFatal("The fake server could not listen.");
    }
    connect(&server, &QTcpServer::newConnection, this, [this] {
      QTcpSocket* socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, this, [this, socket] { serve(socket); });
    });
  }

  QUrl url() const
  {
    return QUrl(QStringLiteral("http://127.0.0.1:%1").arg(server.serverPort()));
  }

  void answer(const QString& methodAndPath, int status, const QByteArray& body, const QByteArray& etag = QByteArray())
  {
    responses.insert(methodAndPath, {status, body, etag});
  }

  QList<Recorded> requests;

private:
  void serve(QTcpSocket* socket)
  {
    buffers[socket].append(socket->readAll());
    const QByteArray& data = buffers[socket];
    const qsizetype headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
      return;
    }
    const QList<QByteArray> lines = data.left(headerEnd).split('\n');
    const QList<QByteArray> requestLine = lines.first().trimmed().split(' ');
    Recorded recorded{requestLine.at(0), requestLine.at(1), QByteArray(), QByteArray(), QByteArray()};
    qsizetype contentLength = 0;
    for (const QByteArray& line : lines.mid(1)) {
      const QByteArray lower = line.trimmed().toLower();
      if (lower.startsWith("content-length:")) {
        contentLength = lower.mid(15).trimmed().toInt();
      }
      if (lower.startsWith("authorization:")) {
        recorded.authorization = line.mid(14).trimmed();
      }
      if (lower.startsWith("if-none-match:")) {
        recorded.ifNoneMatch = line.mid(14).trimmed();
      }
    }
    if (data.size() < headerEnd + 4 + contentLength) {
      return;
    }
    recorded.body = data.mid(headerEnd + 4, contentLength);
    requests.append(recorded);
    const Canned response = responses.value(QString::fromLatin1(recorded.method + ' ' + recorded.path),
      Canned{404, QByteArray("{\"error\":\"not found\"}"), QByteArray()});
    const QByteArray etagHeader = response.etag.isEmpty() ? QByteArray() : "ETag: " + response.etag + "\r\n";
    socket->write("HTTP/1.1 " + QByteArray::number(response.status) + " Reply\r\nContent-Type: application/json\r\n" + etagHeader
      + "Content-Length: " + QByteArray::number(response.body.size()) + "\r\nConnection: close\r\n\r\n" + response.body);
    socket->disconnectFromHost();
    buffers.remove(socket);
  }

  QTcpServer server;
  QMap<QString, Canned> responses;
  QMap<QTcpSocket*, QByteArray> buffers;
};

class AmmoliteClientTest : public QObject {
  Q_OBJECT

private slots:
  void signInSendsTheLoginAndKeepsTheToken()
  {
    FakeServer server;
    server.answer(QStringLiteral("POST /api/v1/auth/token"), 201,
      fixture("signedIn.json"));
    server.answer(QStringLiteral("GET /api/v1/me"), 200, fixture("me.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    SignedInUser user{};
    QString token;
    bool isDone = false;
    connect(&client, &AmmoliteClient::signedIn, this, [&](const SignedInUser& signedIn, const QString& newToken) {
      user = signedIn;
      token = newToken;
      isDone = true;
    });
    client.signIn(QStringLiteral("druid"), QStringLiteral("pass word"), QStringLiteral("Ankh on test"));
    QTRY_VERIFY_WITH_TIMEOUT(isDone, 5000);
    QCOMPARE(user.userID, 9);
    QCOMPARE(user.username, QStringLiteral("druid"));
    QCOMPARE(user.role, QStringLiteral("user"));
    QCOMPARE(token, QStringLiteral("ammolite_abc"));
    QCOMPARE(server.requests.size(), 1);
    QCOMPARE(server.requests.first().authorization, QByteArray());
    const QJsonObject sent = QJsonDocument::fromJson(server.requests.first().body).object();
    QCOMPARE(sent.value(QStringLiteral("username")).toString(), QStringLiteral("druid"));
    QCOMPARE(sent.value(QStringLiteral("password")).toString(), QStringLiteral("pass word"));
    QCOMPARE(sent.value(QStringLiteral("label")).toString(), QStringLiteral("Ankh on test"));

    bool isLoaded = false;
    connect(&client, &AmmoliteClient::meLoaded, this, [&](const SignedInUser&) { isLoaded = true; });
    client.loadMe();
    QTRY_VERIFY_WITH_TIMEOUT(isLoaded, 5000);
    QCOMPARE(server.requests.last().authorization, QByteArray("Bearer ammolite_abc"));
  }

  void aRefusedSignInCarriesTheServersReason()
  {
    FakeServer server;
    server.answer(QStringLiteral("POST /api/v1/auth/token"), 401, fixture("signInRefused.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    QString message;
    bool isSessionExpired = false;
    connect(&client, &AmmoliteClient::signInFailed, this, [&](const QString& reason) { message = reason; });
    connect(&client, &AmmoliteClient::sessionExpired, this, [&] { isSessionExpired = true; });
    client.signIn(QStringLiteral("druid"), QStringLiteral("wrong"), QStringLiteral("Ankh"));
    QTRY_VERIFY_WITH_TIMEOUT(!message.isEmpty(), 5000);
    QCOMPARE(message, QStringLiteral("Invalid username or password."));
    QVERIFY(!isSessionExpired);
  }

  void creatingAnAccountSendsTheInviteAndThenSignsIn()
  {
    FakeServer server;
    server.answer(QStringLiteral("POST /api/v1/register"), 201, fixture("registered.json"));
    server.answer(QStringLiteral("POST /api/v1/auth/token"), 201, fixture("signedIn.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    bool isSignedIn = false;
    connect(&client, &AmmoliteClient::signedIn, this, [&] { isSignedIn = true; });
    client.registerAccount(QStringLiteral("ABCD-EFGH"), QStringLiteral("druid"), QStringLiteral("pw one"), QStringLiteral("pw one"), QStringLiteral("Ankh on test"));
    QTRY_VERIFY_WITH_TIMEOUT(isSignedIn, 5000);
    QCOMPARE(server.requests.size(), 2);
    QCOMPARE(server.requests.at(0).path, QByteArray("/api/v1/register"));
    const QJsonObject registered = QJsonDocument::fromJson(server.requests.at(0).body).object();
    QCOMPARE(registered.value(QStringLiteral("inviteCode")).toString(), QStringLiteral("ABCD-EFGH"));
    QCOMPARE(registered.value(QStringLiteral("username")).toString(), QStringLiteral("druid"));
    QCOMPARE(registered.value(QStringLiteral("password")).toString(), QStringLiteral("pw one"));
    QCOMPARE(registered.value(QStringLiteral("passwordConfirmation")).toString(), QStringLiteral("pw one"));
    QCOMPARE(server.requests.at(1).path, QByteArray("/api/v1/auth/token"));
    const QJsonObject signedIn = QJsonDocument::fromJson(server.requests.at(1).body).object();
    QCOMPARE(signedIn.value(QStringLiteral("username")).toString(), QStringLiteral("druid"));
    QCOMPARE(signedIn.value(QStringLiteral("label")).toString(), QStringLiteral("Ankh on test"));
  }

  void aRefusedInviteIsReportedByTheServersReasonAndNothingSignsIn()
  {
    FakeServer server;
    server.answer(QStringLiteral("POST /api/v1/register"), 400, fixture("inviteRefused.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    QString message;
    bool isSignedIn = false;
    connect(&client, &AmmoliteClient::registerFailed, this, [&](const QString& reason) { message = reason; });
    connect(&client, &AmmoliteClient::signedIn, this, [&] { isSignedIn = true; });
    client.registerAccount(QStringLiteral("nope"), QStringLiteral("druid"), QStringLiteral("pw"), QStringLiteral("pw"), QStringLiteral("Ankh"));
    QTRY_VERIFY_WITH_TIMEOUT(!message.isEmpty(), 5000);
    QCOMPARE(message, QStringLiteral("That invite code is not valid."));
    QVERIFY(!isSignedIn);
    QCOMPARE(server.requests.size(), 1);
  }

  void aSignInThatFailsAfterCreationSaysTheAccountExists()
  {
    FakeServer server;
    server.answer(QStringLiteral("POST /api/v1/register"), 201, fixture("registered.json"));
    server.answer(QStringLiteral("POST /api/v1/auth/token"), 401, fixture("signInRefused.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    QString message;
    bool isSignInFailed = false;
    connect(&client, &AmmoliteClient::registerFailed, this, [&](const QString& reason) { message = reason; });
    connect(&client, &AmmoliteClient::signInFailed, this, [&] { isSignInFailed = true; });
    client.registerAccount(QStringLiteral("ABCD"), QStringLiteral("druid"), QStringLiteral("pw"), QStringLiteral("pw"), QStringLiteral("Ankh"));
    QTRY_VERIFY_WITH_TIMEOUT(!message.isEmpty(), 5000);
    QCOMPARE(message, QStringLiteral("The account was created, but signing in failed: Invalid username or password."));
    QVERIFY(!isSignInFailed);
  }

  void aRevokedTokenEndsTheSession()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 401, fixture("noToken.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_old"));
    bool isSessionExpired = false;
    bool isFailed = false;
    connect(&client, &AmmoliteClient::sessionExpired, this, [&] { isSessionExpired = true; });
    connect(&client, &AmmoliteClient::requestFailed, this, [&] { isFailed = true; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(isSessionExpired, 5000);
    QVERIFY(!isFailed);
  }

  void aPublicManifestCarriesOnlyTheRelease()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifestPublic.json"), "\"etag-1\"");
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    std::optional<Manifest> manifest;
    QByteArray etag;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded, const QByteArray& tag) {
      manifest = loaded;
      etag = tag;
    });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QCOMPARE(server.requests.first().authorization, QByteArray());
    QCOMPARE(server.requests.first().ifNoneMatch, QByteArray());
    QCOMPARE(etag, QByteArray("\"etag-1\""));
    QVERIFY(!manifest->hasCollection);
    QVERIFY(manifest->worlds.isEmpty() && manifest->packages.isEmpty());
    QVERIFY(manifest->ankh.has_value());
    QCOMPARE(manifest->ankh->version, QStringLiteral("2026.11.08.0514"));
    QCOMPARE(manifest->ankh->tag, QStringLiteral("v2026.11.08.0514"));
    QCOMPARE(manifest->ankh->notes, QStringLiteral("- Library toggles"));
    QCOMPARE(manifest->ankh->page, QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/tag/v2026.11.08.0514")));
    QCOMPARE(manifest->ankh->assets.size(), 3);
    QCOMPARE(manifest->ankh->assets.value(QStringLiteral("manifest.json.sig")),
      QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/download/v2026.11.08.0514/manifest.json.sig")));
    QCOMPARE(manifest->ankh->history.size(), 2);
    QCOMPARE(manifest->ankh->history.at(0).version, QStringLiteral("2026.11.08.0514"));
    QCOMPARE(manifest->ankh->history.at(1).version, QStringLiteral("2026.11.08.0334"));
    QCOMPARE(manifest->ankh->history.at(1).notes, QStringLiteral("- Alphabetical library"));
    QCOMPARE(manifest->ankh->history.at(1).publishedAt, QStringLiteral("2026-11-08T03:40:02Z"));
  }

  void aSignedInManifestCarriesTheWorldsAndThePackagesOfTheirEditions()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifest.json"), "\"etag-2\"");
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    std::optional<Manifest> manifest;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded, const QByteArray&) { manifest = loaded; });
    client.loadManifest(QByteArray("\"etag-1\""));
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QCOMPARE(server.requests.first().authorization, QByteArray("Bearer t"));
    QCOMPARE(server.requests.first().ifNoneMatch, QByteArray("\"etag-1\""));
    QVERIFY(manifest->hasCollection);
    QCOMPARE(manifest->worlds.size(), 2);
    const World& arcanite = manifest->worlds.at(0);
    QCOMPARE(arcanite.serverID, 3);
    QCOMPARE(arcanite.name, QStringLiteral("Arcanite"));
    QCOMPARE(arcanite.game, QStringLiteral("World of Warcraft"));
    QCOMPARE(arcanite.variant, QStringLiteral("classic"));
    QCOMPARE(arcanite.version, QStringLiteral("Classic (1.12.1)"));
    QCOMPARE(arcanite.color, QColor("#82b556"));
    QCOMPARE(arcanite.eraStart, 2004);
    QCOMPARE(arcanite.status, QStringLiteral("Online, 12 playing"));
    QCOMPARE(arcanite.accountDomainID, std::optional<int>(2));
    QCOMPARE(arcanite.accountDomain, QStringLiteral("Arcanite Logins"));
    QCOMPARE(arcanite.executable, QStringLiteral("WoW.exe"));
    QVERIFY(arcanite.givesCredential);
    QCOMPARE(manifest->worlds.at(1).executable, QString());
    QVERIFY(!manifest->worlds.at(1).givesCredential);
    QCOMPARE(manifest->worlds.at(1).status, QString());
    QCOMPARE(manifest->worlds.at(1).accountDomainID, std::optional<int>());
    QCOMPARE(manifest->worlds.at(1).accountDomain, QString());
    QCOMPARE(manifest->packages.size(), 2);
    const Package& wowClient = manifest->packages.at(0);
    QCOMPARE(wowClient.name, QStringLiteral("worldofwarcraft-arcanite"));
    QCOMPARE(wowClient.role, QStringLiteral("client"));
    QCOMPARE(wowClient.editions.size(), 1);
    QCOMPARE(wowClient.editions.first().game, QStringLiteral("World of Warcraft"));
    QCOMPARE(wowClient.editions.first().variant, QStringLiteral("classic"));
    QCOMPARE(wowClient.editions.first().label, QStringLiteral("Classic (1.12.1)"));
    QVERIFY(wowClient.latest.has_value());
    QCOMPARE(wowClient.latest->version, QStringLiteral("2026.10.13"));
    QCOMPARE(wowClient.latest->fileName, QStringLiteral("WoW-1.12.1-Arcanite.zip"));
    QCOMPARE(wowClient.latest->byteSize, qint64(5335826980));
    QCOMPARE(wowClient.latest->sha256, QStringLiteral("d298"));
    QCOMPARE(wowClient.latest->fileCount, 159);
    QCOMPARE(wowClient.latest->extractedBytes, qint64(5388545392));
    QCOMPARE(wowClient.latest->url, QUrl(QStringLiteral("https://ammolite.test/api/v1/files/15/WoW-1.12.1-Arcanite.zip")));
    const Package& login = manifest->packages.at(1);
    QCOMPARE(login.role, QStringLiteral("requiredPlugin"));
    QCOMPARE(login.editions.size(), 2);
    QVERIFY(!login.latest.has_value());
    QCOMPARE(wowClient.description, QString());
    QCOMPARE(wowClient.category, QString());
    QCOMPARE(login.category, QStringLiteral("Login"));
    QVERIFY(!wowClient.screenshot.has_value());
    QCOMPARE(login.description, QStringLiteral("Compatibility layer.\nInstalled with the game."));
    QVERIFY(login.screenshot.has_value());
    QCOMPARE(login.screenshot->url, QUrl(QStringLiteral("https://ammolite.test/api/v1/packages/wow-login/screenshot")));
    QCOMPARE(login.screenshot->sha256, QStringLiteral("ab12"));
    QCOMPARE(login.screenshot->contentType, QStringLiteral("image/png"));
    QCOMPARE(manifest->attachedGames.size(), 2);
    const AttachedGame& ddo = manifest->attachedGames.first();
    QCOMPARE(ddo.game, QStringLiteral("Dungeons & Dragons Online"));
    QCOMPARE(ddo.variant, QStringLiteral("live"));
    QCOMPARE(ddo.edition, QStringLiteral("Live"));
    QCOMPARE(ddo.color, QColor("#c0392b"));
    QCOMPARE(ddo.executable, QStringLiteral("x64/dndclient64.exe"));
    QCOMPARE(ddo.launcher, QStringLiteral("turbineGLS"));
    QCOMPARE(ddo.launcherConfig, QStringLiteral("ddo.launcherconfig"));
    QCOMPARE(ddo.steamAppID, 0);
    const AttachedGame& soma = manifest->attachedGames.at(1);
    QCOMPARE(soma.game, QStringLiteral("SOMA"));
    QCOMPARE(soma.executable, QStringLiteral("Soma.exe"));
    QCOMPARE(soma.launcher, QStringLiteral("steam"));
    QCOMPARE(soma.steamAppID, 282140);
    QCOMPARE(soma.launcherConfig, QString());
  }

  void anAttachedGameWhoseExecutableLeavesItsFolderLosesTheCollectionAndKeepsTheRelease()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifestBadExecutable.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_secret"));
    std::optional<Manifest> manifest;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded) { manifest = loaded; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QCOMPARE(manifest->collectionProblem,
      QStringLiteral("the executable of an attached game is not a path inside its folder: ../outside/game.exe"));
    QVERIFY(!manifest->hasCollection);
    QVERIFY(manifest->worlds.isEmpty() && manifest->packages.isEmpty() && manifest->attachedGames.isEmpty());
    QVERIFY(manifest->ankh.has_value());
    QCOMPARE(manifest->ankh->version, QStringLiteral("2026.11.08.0514"));
  }

  void aScreenshotIsFetchedOnceVerifiedAndThenServedFromTheCache()
  {
    const QByteArray image("pretend this is a png");
    const QString hash = QString::fromLatin1(QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex());
    FakeServer server;
    server.answer(QStringLiteral("GET /shot"), 200, image);
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_secret"));
    QTemporaryDir cache;
    const PackageScreenshot screenshot{QUrl(server.url().toString() + QStringLiteral("/shot")), hash, QStringLiteral("image/png")};
    QList<QByteArray> loaded;
    QStringList failures;
    connect(&client, &AmmoliteClient::screenshotLoaded, this, [&](const QString&, const QString&, const QByteArray& bytes) { loaded.append(bytes); });
    connect(&client, &AmmoliteClient::screenshotFailed, this, [&](const QString&, const QString& message) { failures.append(message); });

    client.loadScreenshot(QStringLiteral("wow-login"), screenshot, cache.path());
    QTRY_COMPARE_WITH_TIMEOUT(loaded.size(), 1, 5000);
    QCOMPARE(loaded.first(), image);
    QCOMPARE(server.requests.size(), 1);
    QCOMPARE(server.requests.first().authorization, QByteArray("Bearer ammolite_secret"));
    client.loadScreenshot(QStringLiteral("wow-login"), screenshot, cache.path());
    QTRY_COMPARE_WITH_TIMEOUT(loaded.size(), 2, 5000);
    QCOMPARE(server.requests.size(), 1);

    QFile damaged(QDir(cache.path()).filePath(hash));
    QVERIFY(damaged.open(QIODevice::WriteOnly | QIODevice::Truncate));
    damaged.write("something else");
    damaged.close();
    client.loadScreenshot(QStringLiteral("wow-login"), screenshot, cache.path());
    QTRY_COMPARE_WITH_TIMEOUT(loaded.size(), 3, 5000);
    QCOMPARE(server.requests.size(), 2);
    QCOMPARE(loaded.last(), image);
    QVERIFY(failures.isEmpty());
  }

  void aScreenshotThatDoesNotMatchItsChecksumIsRefusedAndNotCached()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /shot"), 200, QByteArray("tampered"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    QTemporaryDir cache;
    const QString wrongHash = QString::fromLatin1(QCryptographicHash::hash("original", QCryptographicHash::Sha256).toHex());
    QStringList failures;
    bool isLoaded = false;
    connect(&client, &AmmoliteClient::screenshotLoaded, this, [&] { isLoaded = true; });
    connect(&client, &AmmoliteClient::screenshotFailed, this, [&](const QString&, const QString& message) { failures.append(message); });
    client.loadScreenshot(QStringLiteral("wow-login"),
      PackageScreenshot{QUrl(server.url().toString() + QStringLiteral("/shot")), wrongHash, QStringLiteral("image/png")}, cache.path());
    QTRY_COMPARE_WITH_TIMEOUT(failures.size(), 1, 5000);
    QVERIFY(failures.first().contains(QStringLiteral("checksum")));
    QVERIFY(!isLoaded);
    QVERIFY(QDir(cache.path()).isEmpty());
  }

  void anUnchangedManifestIsReportedWithoutAParse()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 304, QByteArray());
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    bool isUnchanged = false;
    bool isLoaded = false;
    bool isFailed = false;
    connect(&client, &AmmoliteClient::manifestUnchanged, this, [&] { isUnchanged = true; });
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&] { isLoaded = true; });
    connect(&client, &AmmoliteClient::requestFailed, this, [&] { isFailed = true; });
    client.loadManifest(QByteArray("\"etag-1\""));
    QTRY_VERIFY_WITH_TIMEOUT(isUnchanged, 5000);
    QCOMPARE(server.requests.first().ifNoneMatch, QByteArray("\"etag-1\""));
    QVERIFY(!isLoaded && !isFailed);
  }

  void aManifestWithoutAReleaseCarriesTheReason()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifestAnkhError.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    std::optional<Manifest> manifest;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded, const QByteArray&) { manifest = loaded; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QVERIFY(!manifest->ankh.has_value());
    QCOMPARE(manifest->ankhProblem, QStringLiteral("GitHub answered 503 for the latest release."));
    QVERIFY(!manifest->hasCollection);
  }

  void aWorldMissingAFieldLosesTheCollectionAndNamesTheField()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifestMissingColor.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    std::optional<Manifest> manifest;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded) { manifest = loaded; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QCOMPARE(manifest->collectionProblem, QStringLiteral("the field \"color\" is missing"));
    QVERIFY(!manifest->hasCollection);
  }

  void aReleaseWithoutTheReleaseListIsRefusedByName()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, "{\"ankh\":{\"version\":\"2026.11.08.0514\",\"tag\":\"v2026.11.08.0514\","
      "\"notes\":\"- Library toggles\",\"page\":\"https://example.test/release\",\"publishedAt\":\"2026-11-08T05:18:56Z\",\"assets\":{}}}");
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    QString message;
    connect(&client, &AmmoliteClient::requestFailed, this, [&](const QString&, const QString& reason) { message = reason; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(!message.isEmpty(), 5000);
    QVERIFY2(message.contains(QStringLiteral("\"releases\" is missing")), qPrintable(message));
  }

  void aPackageWithAnUnknownRoleLosesTheCollectionAndNamesThePackage()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/manifest"), 200, fixture("manifestUnknownRole.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    std::optional<Manifest> manifest;
    connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded) { manifest = loaded; });
    client.loadManifest(QByteArray());
    QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
    QCOMPARE(manifest->collectionProblem, QStringLiteral("the role of the package mystery is unknown: addon"));
    QVERIFY(!manifest->hasCollection);
    QCOMPARE(manifest->ankhProblem, QStringLiteral("GitHub answered 503 for the latest release."));
  }

  void aPackageWhoseCategoryDoesNotFitItsRoleLosesTheCollectionAndNamesThePackage()
  {
    const QList<QPair<QString, QString>> cases = {
      {QStringLiteral("\"role\":\"optionalPlugin\",\"category\":\"Capture\""), QStringLiteral("\"Capture\"")},
      {QStringLiteral("\"role\":\"requiredPlugin\",\"category\":\"\""), QStringLiteral("\"\"")},
      {QStringLiteral("\"role\":\"client\",\"category\":\"Login\""), QStringLiteral("\"Login\"")},
    };
    for (const auto& [fields, shown] : cases) {
      FakeServer server;
      server.answer(QStringLiteral("GET /api/v1/manifest"), 200, QStringLiteral("{\"ankh\":{\"error\":\"none\"},\"worlds\":[],"
        "\"packages\":[{\"name\":\"mystery\",\"label\":\"Mystery\",%1,\"description\":\"\",\"screenshot\":null,\"editions\":[],"
        "\"latest\":null}],\"attachedGames\":[]}").arg(fields).toUtf8());
      AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
      client.setToken(QStringLiteral("t"));
      std::optional<Manifest> manifest;
      connect(&client, &AmmoliteClient::manifestLoaded, this, [&](const Manifest& loaded) { manifest = loaded; });
      client.loadManifest(QByteArray());
      QTRY_VERIFY_WITH_TIMEOUT(manifest.has_value(), 5000);
      QCOMPARE(manifest->collectionProblem, QStringLiteral("the category of the package mystery does not fit its role: %1").arg(shown));
      QVERIFY(!manifest->hasCollection);
    }
  }

  void gameAccountsAreParsedWithTheirLogins()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/me/gameAccounts"), 200, fixture("gameAccounts.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    QList<AccountDomain> domains;
    connect(&client, &AmmoliteClient::gameAccountsLoaded, this, [&](const QList<AccountDomain>& loaded) { domains = loaded; });
    client.loadGameAccounts();
    QTRY_COMPARE_WITH_TIMEOUT(domains.size(), 1, 5000);
    QCOMPARE(domains.first().accountDomainID, 2);
    QCOMPARE(domains.first().capabilities, QStringList({QStringLiteral("createAccount"), QStringLiteral("changePassword")}));
    QCOMPARE(domains.first().logins.size(), 1);
    QCOMPARE(domains.first().logins.first().gameAccountID, 11);
    QCOMPARE(domains.first().logins.first().gameUsername, QStringLiteral("thrall"));
  }

  void signingOutRevokesTheTokenOnTheServer()
  {
    FakeServer server;
    server.answer(QStringLiteral("DELETE /api/v1/auth/token"), 200, fixture("revoked.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_abc"));
    std::optional<QString> warning;
    connect(&client, &AmmoliteClient::signedOut, this, [&](const QString& text) { warning = text; });
    client.signOut();
    QTRY_VERIFY_WITH_TIMEOUT(warning.has_value(), 5000);
    QCOMPARE(*warning, QString());
    QCOMPARE(server.requests.first().authorization, QByteArray("Bearer ammolite_abc"));
  }

  void anUnreachableServerIsReportedWhenSigningOut()
  {
    AmmoliteClient client(QUrl(QStringLiteral("http://127.0.0.1:9")), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_abc"));
    std::optional<QString> warning;
    connect(&client, &AmmoliteClient::signedOut, this, [&](const QString& text) { warning = text; });
    client.signOut();
    QTRY_VERIFY_WITH_TIMEOUT(warning.has_value(), 10000);
    QVERIFY2(warning->startsWith(QStringLiteral("The sign-in could not be revoked on the server")), qPrintable(*warning));
  }

  void serviceAddressesMustBeWholeOriginsOverHttpsOrLoopback()
  {
    QCOMPARE(validatedServiceUrl(QStringLiteral("https://ammolite.unseencolor.com:1996")).port(), 1996);
    QCOMPARE(validatedServiceUrl(QStringLiteral("http://127.0.0.1:8123")).host(), QStringLiteral("127.0.0.1"));
  }
};

QTEST_GUILESS_MAIN(AmmoliteClientTest)
#include "ammoliteClientTest.moc"
