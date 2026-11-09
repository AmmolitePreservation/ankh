#include "turbineLauncher.h"

#include <QDir>
#include <QFile>
#include <QMap>
#include <QNetworkAccessManager>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

namespace {

class FakeGls : public QObject {
public:
  struct Recorded {
    QByteArray method;
    QByteArray path;
    QByteArray soapAction;
    QByteArray body;
  };

  FakeGls()
  {
    if (!server.listen(QHostAddress::LocalHost, 0)) {
      qFatal("The fake GLS server could not listen.");
    }
    connect(&server, &QTcpServer::newConnection, this, [this] {
      QTcpSocket* socket = server.nextPendingConnection();
      connect(socket, &QTcpSocket::readyRead, this, [this, socket] { serve(socket); });
    });
  }

  QString url(const QString& path) const
  {
    return QStringLiteral("http://127.0.0.1:%1%2").arg(server.serverPort()).arg(path);
  }

  void answer(const QString& path, int status, const QByteArray& body)
  {
    responses.insert(path, {status, body});
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
      if (lower.startsWith("soapaction:")) {
        recorded.soapAction = line.mid(11).trimmed();
      }
    }
    if (data.size() < headerEnd + 4 + contentLength) {
      return;
    }
    recorded.body = data.mid(headerEnd + 4, contentLength);
    requests.append(recorded);
    const QPair<int, QByteArray> response = responses.value(QString::fromLatin1(recorded.path), {404, QByteArray("not found")});
    socket->write("HTTP/1.1 " + QByteArray::number(response.first) + " Reply\r\nContent-Type: text/xml\r\nContent-Length: "
      + QByteArray::number(response.second.size()) + "\r\nConnection: close\r\n\r\n" + response.second);
    socket->disconnectFromHost();
    buffers.remove(socket);
  }

  QTcpServer server;
  QMap<QString, QPair<int, QByteArray>> responses;
  QMap<QTcpSocket*, QByteArray> buffers;
};

QByteArray soap(const QByteArray& inner)
{
  return "<?xml version=\"1.0\" encoding=\"utf-8\"?><soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">"
    "<soap:Body>" + inner + "</soap:Body></soap:Envelope>";
}

QByteArray appSettings(const QList<QPair<QByteArray, QByteArray>>& entries)
{
  QByteArray text = "<?xml version=\"1.0\" encoding=\"utf-8\"?><configuration><appSettings>";
  for (const auto& [key, value] : entries) {
    text += "<add key=\"" + key + "\" value=\"" + QByteArray(value).replace("&", "&amp;") + "\"/>";
  }
  return text + "</appSettings></configuration>";
}

struct Harness {
  FakeGls gls;
  QTemporaryDir game;
  QNetworkAccessManager network;

  Harness()
  {
    QFile config(QDir(game.path()).filePath(QStringLiteral("alpha.launcherconfig")));
    config.open(QIODevice::WriteOnly);
    config.write(appSettings({{"Launcher.DataCenterService.GLS", gls.url(QStringLiteral("/GLS/Service.asmx")).toUtf8()},
      {"DataCenter.GameName", "ALPHA"}}));
    config.close();
    gls.answer(QStringLiteral("/GLS/Service.asmx"), 200, soap(
      "<GetDatacentersResponse xmlns=\"http://www.turbine.com/SE/GLS\"><GetDatacentersResult>"
      "<Datacenter><Name>OTHER</Name><AuthServer>http://x/</AuthServer><LauncherConfigurationServer>http://x/</LauncherConfigurationServer><Worlds/></Datacenter>"
      "<Datacenter><Name>ALPHA</Name><AuthServer>" + gls.url(QStringLiteral("/Auth.asmx")).toUtf8() + "</AuthServer>"
      "<PatchServer>patch.example</PatchServer>"
      "<LauncherConfigurationServer>" + gls.url(QStringLiteral("/launcher.server.config")).toUtf8() + "</LauncherConfigurationServer>"
      "<Worlds>"
      "<World><Name>Second</Name><ChatServerUrl>chat2.example:2900</ChatServerUrl><StatusServerUrl>" + gls.url(QStringLiteral("/status/second")).toUtf8() + "</StatusServerUrl><Order>2</Order></World>"
      "<World><Name>First [US]</Name><ChatServerUrl>chat1.example:2900</ChatServerUrl><StatusServerUrl>" + gls.url(QStringLiteral("/status/first")).toUtf8() + "</StatusServerUrl><Order>1</Order></World>"
      "</Worlds></Datacenter></GetDatacentersResult></GetDatacentersResponse>"));
    gls.answer(QStringLiteral("/launcher.server.config"), 200, appSettings({
      {"GameClient.WIN64.Filename", "alpha64.exe"},
      {"GameClient.WIN32.ArgTemplate", "-a {SUBSCRIPTION} -h {LOGIN} --glsticketdirect {GLS} --chatserver {CHAT} --language {LANG} --authserverurl {AUTHSERVERURL} --rodat on"},
      {"GameClient.Arg.authserverurl", "https://auth.example/"},
      {"WorldQueue.LoginQueue.URL", gls.url(QStringLiteral("/queue")).toUtf8()},
      {"WorldQueue.TakeANumber.Parameters", "command=TakeANumber&subscription={0}&ticket={1}&ticket_type=GLS&queue_url={2}"}}));
    gls.answer(QStringLiteral("/Auth.asmx"), 200, soap(
      "<LoginAccountResponse xmlns=\"http://www.turbine.com/SE/GLS\"><LoginAccountResult><Ticket>TICKET-123</Ticket>"
      "<Subscriptions><GameSubscription><Game>OTHER</Game><Name>other.sub</Name></GameSubscription>"
      "<GameSubscription><Game>ALPHA</Game><Name>alpha.sub.7</Name><Status>Active</Status></GameSubscription></Subscriptions>"
      "</LoginAccountResult></LoginAccountResponse>"));
    gls.answer(QStringLiteral("/status/first"), 200,
      "<Status><name>First [US]</name><loginservers>10.0.0.1:9004;10.0.0.2:9004;</loginservers><queueurls>" + gls.url(QStringLiteral("/queue/first")).toUtf8()
      + ";</queueurls><world_full>false</world_full><nowservingqueuenumber>0x1A</nowservingqueuenumber><lastassignedqueuenumber>0x1A</lastassignedqueuenumber></Status>");
    gls.answer(QStringLiteral("/queue"), 200, "<Result><HResult>0x00000000</HResult><QueueNumber>0x1B</QueueNumber><NowServingNumber>0x1B</NowServingNumber></Result>");
  }
};

struct Outcome {
  QStringList statuses;
  QStringList worlds;
  QString executable;
  QStringList arguments;
  QString failure;
  bool isReady = false;
  bool isListed = false;
};

void drive(Harness& harness, Outcome& outcome, const std::function<void(TurbineLauncher&)>& act)
{
  TurbineLauncher launcher(&harness.network);
  QObject::connect(&launcher, &TurbineLauncher::statusChanged, &launcher, [&](const QString& text) { outcome.statuses.append(text); });
  QObject::connect(&launcher, &TurbineLauncher::worldsListed, &launcher, [&](const QList<TurbineWorld>& worlds) {
    for (const TurbineWorld& world : worlds) {
      outcome.worlds.append(world.name);
    }
    outcome.isListed = true;
  });
  QObject::connect(&launcher, &TurbineLauncher::launchReady, &launcher, [&](const QString& executable, const QStringList& arguments) {
    outcome.executable = executable;
    outcome.arguments = arguments;
    outcome.isReady = true;
  });
  QObject::connect(&launcher, &TurbineLauncher::failed, &launcher, [&](const QString& message) { outcome.failure = message; });
  act(launcher);
  QTRY_VERIFY_WITH_TIMEOUT(outcome.isReady || outcome.isListed || !outcome.failure.isEmpty(), 10000);
}

}

class TurbineLauncherTest : public QObject {
  Q_OBJECT

private slots:
  void theWorldsComeFromTheDatacenterInOrder()
  {
    Harness harness;
    Outcome outcome;
    drive(harness, outcome, [&](TurbineLauncher& launcher) { launcher.listWorlds(harness.game.path(), QStringLiteral("alpha.launcherconfig")); });
    QVERIFY2(outcome.failure.isEmpty(), qPrintable(outcome.failure));
    QCOMPARE(outcome.worlds, (QStringList{QStringLiteral("First [US]"), QStringLiteral("Second")}));
    QCOMPARE(harness.gls.requests.size(), 1);
    QCOMPARE(harness.gls.requests.first().soapAction, QByteArray("http://www.turbine.com/SE/GLS/GetDatacenters"));
    QVERIFY(harness.gls.requests.first().body.contains("<game xmlns=\"http://www.turbine.com/SE/GLS\">ALPHA</game>")
      || harness.gls.requests.first().body.contains(">ALPHA</"));
  }

  void aLaunchWalksTheLauncherStepsAndBuildsTheClientArguments()
  {
    Harness harness;
    Outcome outcome;
    drive(harness, outcome, [&](TurbineLauncher& launcher) {
      launcher.prepareLaunch(harness.game.path(), QStringLiteral("alpha.launcherconfig"), QStringLiteral("First [US]"),
        QStringLiteral("player7"), QStringLiteral("p&ss<word>"), QStringLiteral("English"));
    });
    QVERIFY2(outcome.failure.isEmpty(), qPrintable(outcome.failure));
    QCOMPARE(outcome.executable, QStringLiteral("alpha64.exe"));
    QCOMPARE(outcome.arguments, (QStringList{QStringLiteral("-a"), QStringLiteral("alpha.sub.7"), QStringLiteral("-h"), QStringLiteral("10.0.0.1:9004"),
      QStringLiteral("--glsticketdirect"), QStringLiteral("TICKET-123"), QStringLiteral("--chatserver"), QStringLiteral("chat1.example:2900"),
      QStringLiteral("--language"), QStringLiteral("English"), QStringLiteral("--authserverurl"), QStringLiteral("https://auth.example/"),
      QStringLiteral("--rodat"), QStringLiteral("on")}));
    QStringList paths;
    for (const FakeGls::Recorded& request : harness.gls.requests) {
      paths.append(QString::fromLatin1(request.path));
    }
    QCOMPARE(paths, (QStringList{QStringLiteral("/GLS/Service.asmx"), QStringLiteral("/launcher.server.config"), QStringLiteral("/Auth.asmx"),
      QStringLiteral("/status/first"), QStringLiteral("/queue")}));
    const QByteArray login = harness.gls.requests.at(2).body;
    QVERIFY2(login.contains("<username xmlns=\"http://www.turbine.com/SE/GLS\">player7</username>") || login.contains(">player7</username>"), login.constData());
    QVERIFY2(login.contains("p&amp;ss&lt;word"), login.constData());
    QCOMPARE(harness.gls.requests.at(2).soapAction, QByteArray("http://www.turbine.com/SE/GLS/LoginAccount"));
    const QByteArray queue = harness.gls.requests.at(4).body;
    QVERIFY2(queue.contains("command=TakeANumber&subscription=alpha.sub.7&ticket=TICKET-123&ticket_type=GLS&queue_url=http%3A%2F%2F127.0.0.1"), queue.constData());
    QCOMPARE(outcome.statuses.size(), 5);
    QVERIFY(outcome.statuses.at(2).contains(QStringLiteral("Signing player7 in")));
  }

  void aWrongPasswordIsTheServicesFaultMessage()
  {
    Harness harness;
    harness.gls.answer(QStringLiteral("/Auth.asmx"), 500, soap(
      "<soap:Fault><faultcode>soap:Server</faultcode><faultstring>Account name or password is incorrect.</faultstring></soap:Fault>"));
    Outcome outcome;
    drive(harness, outcome, [&](TurbineLauncher& launcher) {
      launcher.prepareLaunch(harness.game.path(), QStringLiteral("alpha.launcherconfig"), QStringLiteral("First [US]"),
        QStringLiteral("player7"), QStringLiteral("wrong"), QStringLiteral("English"));
    });
    QCOMPARE(outcome.failure, QStringLiteral("Account name or password is incorrect."));
    QVERIFY(!outcome.isReady);
  }

  void aFullWorldARefusingQueueAndAnUnknownWorldAreNamed()
  {
    Harness harness;
    Outcome unknown;
    drive(harness, unknown, [&](TurbineLauncher& launcher) {
      launcher.prepareLaunch(harness.game.path(), QStringLiteral("alpha.launcherconfig"), QStringLiteral("Nowhere"), QStringLiteral("p"), QStringLiteral("w"), QStringLiteral("English"));
    });
    QCOMPARE(unknown.failure, QStringLiteral("There is no world named Nowhere; the worlds are First [US], Second."));

    harness.gls.answer(QStringLiteral("/status/first"), 200,
      "<Status><name>First [US]</name><loginservers>10.0.0.1:9004;</loginservers><queueurls>q;</queueurls><world_full>true</world_full></Status>");
    Outcome full;
    drive(harness, full, [&](TurbineLauncher& launcher) {
      launcher.prepareLaunch(harness.game.path(), QStringLiteral("alpha.launcherconfig"), QStringLiteral("First [US]"), QStringLiteral("p"), QStringLiteral("w"), QStringLiteral("English"));
    });
    QCOMPARE(full.failure, QStringLiteral("First [US] is full right now."));

    harness.gls.answer(QStringLiteral("/status/first"), 200,
      "<Status><name>First [US]</name><loginservers>10.0.0.1:9004;</loginservers><queueurls>q;</queueurls><world_full>false</world_full></Status>");
    harness.gls.answer(QStringLiteral("/queue"), 200, "<Result><HResult>0x80004005</HResult></Result>");
    Outcome refused;
    drive(harness, refused, [&](TurbineLauncher& launcher) {
      launcher.prepareLaunch(harness.game.path(), QStringLiteral("alpha.launcherconfig"), QStringLiteral("First [US]"), QStringLiteral("p"), QStringLiteral("w"), QStringLiteral("English"));
    });
    QCOMPARE(refused.failure, QStringLiteral("The login queue refused the account (HRESULT 0x80004005)."));
  }

  void aMissingLauncherConfigurationIsNamed()
  {
    Harness harness;
    Outcome outcome;
    drive(harness, outcome, [&](TurbineLauncher& launcher) { launcher.listWorlds(harness.game.path(), QStringLiteral("missing.launcherconfig")); });
    QVERIFY2(outcome.failure.contains(QStringLiteral("missing.launcherconfig could not be read")), qPrintable(outcome.failure));
  }
};

QTEST_GUILESS_MAIN(TurbineLauncherTest)
#include "turbineLauncherTest.moc"
