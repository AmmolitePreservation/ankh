#include "ammoliteClient.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
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
    QByteArray body;
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

  void answer(const QString& methodAndPath, int status, const QByteArray& body)
  {
    responses.insert(methodAndPath, {status, body});
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
    Recorded recorded{requestLine.at(0), requestLine.at(1), QByteArray(), QByteArray()};
    qsizetype contentLength = 0;
    for (const QByteArray& line : lines.mid(1)) {
      const QByteArray lower = line.trimmed().toLower();
      if (lower.startsWith("content-length:")) {
        contentLength = lower.mid(15).trimmed().toInt();
      }
      if (lower.startsWith("authorization:")) {
        recorded.authorization = line.mid(14).trimmed();
      }
    }
    if (data.size() < headerEnd + 4 + contentLength) {
      return;
    }
    recorded.body = data.mid(headerEnd + 4, contentLength);
    requests.append(recorded);
    const auto response = responses.value(QString::fromLatin1(recorded.method + ' ' + recorded.path), {404, QByteArray("{\"error\":\"not found\"}")});
    socket->write("HTTP/1.1 " + QByteArray::number(response.first) + " Reply\r\nContent-Type: application/json\r\nContent-Length: "
      + QByteArray::number(response.second.size()) + "\r\nConnection: close\r\n\r\n" + response.second);
    socket->disconnectFromHost();
    buffers.remove(socket);
  }

  QTcpServer server;
  QMap<QString, QPair<int, QByteArray>> responses;
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
    server.answer(QStringLiteral("GET /api/v1/worlds"), 401, fixture("noToken.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("ammolite_old"));
    bool isSessionExpired = false;
    bool isFailed = false;
    connect(&client, &AmmoliteClient::sessionExpired, this, [&] { isSessionExpired = true; });
    connect(&client, &AmmoliteClient::requestFailed, this, [&] { isFailed = true; });
    client.loadWorlds();
    QTRY_VERIFY_WITH_TIMEOUT(isSessionExpired, 5000);
    QVERIFY(!isFailed);
  }

  void worldsAreParsedWithTheirOptionalFields()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/worlds"), 200, fixture("worlds.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    QList<World> worlds;
    connect(&client, &AmmoliteClient::worldsLoaded, this, [&](const QList<World>& loaded) { worlds = loaded; });
    client.loadWorlds();
    QTRY_COMPARE_WITH_TIMEOUT(worlds.size(), 2, 5000);
    QCOMPARE(worlds.at(0).serverID, 3);
    QCOMPARE(worlds.at(0).name, QStringLiteral("Arcanite"));
    QCOMPARE(worlds.at(0).game, QStringLiteral("World of Warcraft"));
    QCOMPARE(worlds.at(0).version, QStringLiteral("Classic (1.12.1)"));
    QCOMPARE(worlds.at(0).color, QColor("#82b556"));
    QCOMPARE(worlds.at(0).eraStart, 2004);
    QCOMPARE(worlds.at(0).status, QStringLiteral("Online, 12 playing"));
    QCOMPARE(worlds.at(0).accountDomainID, std::optional<int>(2));
    QCOMPARE(worlds.at(0).accountDomain, QStringLiteral("Arcanite Logins"));
    QCOMPARE(worlds.at(1).status, QString());
    QCOMPARE(worlds.at(1).accountDomainID, std::optional<int>());
    QCOMPARE(worlds.at(1).accountDomain, QString());
  }

  void aWorldMissingAFieldIsRefusedByName()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/worlds"), 200, fixture("worldMissingColor.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    QString what;
    QString message;
    bool isLoaded = false;
    connect(&client, &AmmoliteClient::worldsLoaded, this, [&] { isLoaded = true; });
    connect(&client, &AmmoliteClient::requestFailed, this, [&](const QString& failed, const QString& reason) {
      what = failed;
      message = reason;
    });
    client.loadWorlds();
    QTRY_VERIFY_WITH_TIMEOUT(!what.isEmpty(), 5000);
    QCOMPARE(what, QStringLiteral("worlds"));
    QVERIFY2(message.contains(QStringLiteral("\"color\" is missing")), qPrintable(message));
    QVERIFY(!isLoaded);
  }

  void downloadsAreParsedPerCategory()
  {
    FakeServer server;
    server.answer(QStringLiteral("GET /api/v1/worlds/3"), 200, fixture("worldDetail.json"));
    AmmoliteClient client(server.url(), QStringLiteral("2026.11.08.0100"));
    client.setToken(QStringLiteral("t"));
    int serverID = 0;
    QList<DownloadGroup> groups;
    connect(&client, &AmmoliteClient::worldLoaded, this, [&](int id, const QList<DownloadGroup>& loaded) {
      serverID = id;
      groups = loaded;
    });
    client.loadWorld(3);
    QTRY_COMPARE_WITH_TIMEOUT(groups.size(), 1, 5000);
    QCOMPARE(serverID, 3);
    QCOMPARE(groups.first().category, QStringLiteral("Client Files"));
    QCOMPARE(groups.first().packages.size(), 1);
    const Download& download = groups.first().packages.first();
    QCOMPARE(download.packageVersionID, 15);
    QCOMPARE(download.fileName, QStringLiteral("WoW-1.12.1-Arcanite.zip"));
    QCOMPARE(download.byteSize, qint64(5335826980));
    QCOMPARE(download.sha256, QStringLiteral("d298"));
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
