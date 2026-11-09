#include "installJob.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QtTest>

namespace {

QByteArray fixture(const char* name)
{
  QFile file(QStringLiteral(ANKH_FIXTURE_DIR "/install/") + QLatin1String(name));
  if (!file.open(QIODevice::ReadOnly)) {
    qFatal("Missing fixture %s", name);
  }
  return file.readAll();
}

QString sha256Of(const QByteArray& bytes)
{
  return QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
}

QByteArray contentOf(const QString& path)
{
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    return QByteArray();
  }
  return file.readAll();
}

void writeFile(const QString& path, const QByteArray& content)
{
  QDir().mkpath(QFileInfo(path).absolutePath());
  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    qFatal("Could not write %s", qPrintable(path));
  }
  file.write(content);
}

}

// Serves canned bodies over HTTP, honoring Range unless told to ignore it, and can hold a download open after a
// few bytes so a cancel can be tested mid-transfer.
class FakeFileServer : public QObject {
public:
  struct Recorded {
    QByteArray method;
    QByteArray path;
    QByteArray authorization;
    QByteArray range;
  };

  struct Canned {
    int status;
    QByteArray body;
    QByteArray contentType;
  };

  FakeFileServer()
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

  void answer(const QString& path, int status, const QByteArray& body, const QByteArray& contentType)
  {
    responses.insert(path, {status, body, contentType});
  }

  QList<Recorded> requests;
  bool ignoresRange = false;
  qint64 holdAfterBytes = -1;

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
    for (const QByteArray& line : lines.mid(1)) {
      const QByteArray lower = line.trimmed().toLower();
      if (lower.startsWith("authorization:")) {
        recorded.authorization = line.mid(14).trimmed();
      }
      if (lower.startsWith("range:")) {
        recorded.range = line.mid(6).trimmed();
      }
    }
    requests.append(recorded);
    buffers.remove(socket);
    const Canned response = responses.value(QString::fromLatin1(recorded.path), Canned{404, "{\"error\":\"not found\"}", "application/json"});
    QByteArray body = response.body;
    int status = response.status;
    QByteArray extraHeaders;
    if (status == 200 && !recorded.range.isEmpty() && !ignoresRange) {
      const qint64 start = recorded.range.mid(6).chopped(1).toLongLong();
      if (start >= body.size()) {
        status = 416;
        body.clear();
      } else {
        status = 206;
        extraHeaders = "Content-Range: bytes " + QByteArray::number(start) + "-" + QByteArray::number(body.size() - 1) + "/"
          + QByteArray::number(body.size()) + "\r\n";
        body = body.mid(start);
      }
    }
    socket->write("HTTP/1.1 " + QByteArray::number(status) + " Reply\r\nContent-Type: " + response.contentType + "\r\n" + extraHeaders
      + "Accept-Ranges: bytes\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\nConnection: close\r\n\r\n");
    if (holdAfterBytes >= 0 && response.contentType == "application/octet-stream") {
      socket->write(body.left(holdAfterBytes));
      return;
    }
    socket->write(body);
    socket->disconnectFromHost();
  }

  QTcpServer server;
  QMap<QString, Canned> responses;
  QMap<QTcpSocket*, QByteArray> buffers;
};

class InstallJobTest : public QObject {
  Q_OBJECT

  const QByteArray clientZip = fixture("client.zip");
  const QByteArray client2Zip = fixture("client2.zip");
  const QByteArray toolZip = fixture("tool.zip");
  const QByteArray mapsZip = fixture("maps.zip");
  const QByteArray mapsListing = "{\"files\":["
    "{\"path\":\"maps/world.map\",\"byteSize\":400,\"crc32\":901,\"writeOnce\":false},"
    "{\"path\":\"maps/maps.ini\",\"byteSize\":19,\"crc32\":902,\"writeOnce\":true}]}";
  const QByteArray clientListing = "{\"files\":["
    "{\"path\":\"Game.exe\",\"byteSize\":200,\"crc32\":111,\"writeOnce\":false},"
    "{\"path\":\"Data/world.dat\",\"byteSize\":2400,\"crc32\":222,\"writeOnce\":false},"
    "{\"path\":\"readme.txt\",\"byteSize\":13,\"crc32\":333,\"writeOnce\":false},"
    "{\"path\":\"settings.ini\",\"byteSize\":20,\"crc32\":444,\"writeOnce\":true}]}";
  const QByteArray client2Listing = "{\"files\":["
    "{\"path\":\"Game.exe\",\"byteSize\":300,\"crc32\":555,\"writeOnce\":false},"
    "{\"path\":\"Data/world.dat\",\"byteSize\":2400,\"crc32\":222,\"writeOnce\":false},"
    "{\"path\":\"Data/extra.dat\",\"byteSize\":500,\"crc32\":666,\"writeOnce\":false},"
    "{\"path\":\"settings.ini\",\"byteSize\":30,\"crc32\":777,\"writeOnce\":true}]}";
  const QByteArray toolListing = "{\"files\":[{\"path\":\"tools/directConnect.dll\",\"byteSize\":1100,\"crc32\":888,\"writeOnce\":false}]}";

  struct Outcome {
    QStringList statuses;
    QList<QPair<qint64, qint64>> progress;
    bool isDone = false;
    bool isFailed = false;
    bool isCancelled = false;
    QString failure;
    Receipt receipt;
    ApplySummary summary;
  };

  Game alpha() const
  {
    return {QStringLiteral("1"), 1, QStringLiteral("Alpha"), QStringLiteral("Classic"), QStringLiteral("classic"),
      QStringLiteral("Alpha Prime"), QStringLiteral("AL"), QColor("#336699"), QStringLiteral("Online"), std::nullopt, QString(), false};
  }

  Package clientPackage(const FakeFileServer& server) const
  {
    return {QStringLiteral("alpha-client"), QStringLiteral("Alpha client"), QStringLiteral("client"),
      {{QStringLiteral("Alpha"), QStringLiteral("classic"), QStringLiteral("Classic")}},
      PackageVersion{QStringLiteral("2026.11.01"), QStringLiteral("alphaClient.zip"), clientZip.size(), sha256Of(clientZip),
        QStringLiteral("2026-11-01T00:00:00Z"), 4, 2633, QUrl(server.url().toString() + QStringLiteral("/files/client.zip"))}};
  }

  Package client2Package(const FakeFileServer& server) const
  {
    Package package = clientPackage(server);
    package.latest = PackageVersion{QStringLiteral("2026.11.09"), QStringLiteral("alphaClient2.zip"), client2Zip.size(), sha256Of(client2Zip),
      QStringLiteral("2026-11-09T00:00:00Z"), 4, 3230, QUrl(server.url().toString() + QStringLiteral("/files/client2.zip"))};
    return package;
  }

  Package toolPackage(const FakeFileServer& server) const
  {
    return {QStringLiteral("alpha-tools"), QStringLiteral("Alpha tools"), QStringLiteral("requiredTool"),
      {{QStringLiteral("Alpha"), QStringLiteral("classic"), QStringLiteral("Classic")}},
      PackageVersion{QStringLiteral("2026.11.02-abc1234"), QStringLiteral("alphaTools.zip"), toolZip.size(), sha256Of(toolZip),
        QStringLiteral("2026-11-02T00:00:00Z"), 1, 1100, QUrl(server.url().toString() + QStringLiteral("/files/tool.zip"))}};
  }

  Package optionalPackage() const
  {
    return {QStringLiteral("alpha-maps"), QStringLiteral("Alpha maps"), QStringLiteral("optionalTool"),
      {{QStringLiteral("Alpha"), QStringLiteral("classic"), QStringLiteral("Classic")}}, std::nullopt};
  }

  Game ownCopy() const
  {
    Game game = alpha();
    game.id = QStringLiteral("attached/Alpha/classic");
    game.isAttached = true;
    game.executable = QStringLiteral("bin/alpha.exe");
    return game;
  }

  Package mapsPackage(const FakeFileServer& server) const
  {
    return {QStringLiteral("alpha-maps"), QStringLiteral("Alpha maps"), QStringLiteral("optionalTool"),
      {{QStringLiteral("Alpha"), QStringLiteral("classic"), QStringLiteral("Classic")}},
      PackageVersion{QStringLiteral("2026.11.03"), QStringLiteral("alphaMaps.zip"), mapsZip.size(), sha256Of(mapsZip),
        QStringLiteral("2026-11-03T00:00:00Z"), 2, 419, QUrl(server.url().toString() + QStringLiteral("/files/maps.zip"))}};
  }

  void serveEverything(FakeFileServer& server)
  {
    server.answer(QStringLiteral("/api/v1/packages/alpha-maps/versions/2026.11.03/files"), 200, mapsListing, "application/json");
    server.answer(QStringLiteral("/files/maps.zip"), 200, mapsZip, "application/octet-stream");
    server.answer(QStringLiteral("/api/v1/packages/alpha-client/versions/2026.11.01/files"), 200, clientListing, "application/json");
    server.answer(QStringLiteral("/api/v1/packages/alpha-client/versions/2026.11.09/files"), 200, client2Listing, "application/json");
    server.answer(QStringLiteral("/api/v1/packages/alpha-tools/versions/2026.11.02-abc1234/files"), 200, toolListing, "application/json");
    server.answer(QStringLiteral("/files/client.zip"), 200, clientZip, "application/octet-stream");
    server.answer(QStringLiteral("/files/client2.zip"), 200, client2Zip, "application/octet-stream");
    server.answer(QStringLiteral("/files/tool.zip"), 200, toolZip, "application/octet-stream");
  }

  InstallPlan planFor(const FakeFileServer& server, const QString& libraryRoot)
  {
    QString problem;
    const std::optional<InstallPlan> plan = planInstall(alpha(), {clientPackage(server), toolPackage(server), optionalPackage()}, libraryRoot, problem);
    if (!plan) {
      qFatal("%s", qPrintable(problem));
    }
    return *plan;
  }

  void run(InstallJob& job, Outcome& outcome, const std::function<void(Outcome&)>& during = {})
  {
    connect(&job, &InstallJob::statusChanged, this, [&outcome](const QString& text) { outcome.statuses.append(text); });
    connect(&job, &InstallJob::progressChanged, this, [&outcome](qint64 done, qint64 total) { outcome.progress.append({done, total}); });
    connect(&job, &InstallJob::finished, this, [&outcome](const Receipt& receipt, const ApplySummary& summary) {
      outcome.receipt = receipt;
      outcome.summary = summary;
      outcome.isDone = true;
    });
    connect(&job, &InstallJob::failed, this, [&outcome](const QString& message) {
      outcome.failure = message;
      outcome.isFailed = true;
    });
    connect(&job, &InstallJob::cancelled, this, [&outcome] { outcome.isCancelled = true; });
    job.start();
    if (during) {
      during(outcome);
    }
    QTRY_VERIFY_WITH_TIMEOUT(outcome.isDone || outcome.isFailed || outcome.isCancelled, 20000);
  }

  Receipt installFirstVersion(FakeFileServer& server, const QString& libraryRoot)
  {
    InstallJob job(planFor(server, libraryRoot), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    if (!outcome.isDone) {
      qFatal("The first install failed: %s", qPrintable(outcome.failure));
    }
    return outcome.receipt;
  }

private slots:
  void aPlanTakesTheClientAndRequiredToolsAndNamesTheFolder()
  {
    FakeFileServer server;
    const InstallPlan plan = planFor(server, QStringLiteral("D:/Games/clients"));
    QVERIFY(!plan.isUpdate());
    QCOMPARE(plan.client.name, QStringLiteral("alpha-client"));
    QCOMPARE(plan.tools.size(), 1);
    QCOMPARE(plan.tools.first().name, QStringLiteral("alpha-tools"));
    QCOMPARE(plan.fetch.size(), 2);
    QCOMPARE(plan.folder, QStringLiteral("D:\\Games\\clients\\Alpha - Classic"));
    QCOMPARE(plan.downloadBytes(), qint64(clientZip.size() + toolZip.size()));
    QCOMPARE(plan.extractedBytes(), qint64(3733));
    QCOMPARE(plan.fileCount(), 5);
    QCOMPARE(stagedZipPath(QStringLiteral("D:/Games/clients"), plan.client), QStringLiteral("D:\\Games\\clients\\.staging\\alpha-client-2026.11.01.zip"));
    QCOMPARE(stagingRootFor(plan), QStringLiteral("D:\\Games\\clients"));
  }

  void aGameInstalledSomewhereElseStagesBesideItsOwnFolder()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    QTemporaryDir elsewhere;
    QString problem;
    const std::optional<InstallPlan> plan = planInstall(alpha(), {clientPackage(server), toolPackage(server)}, elsewhere.path(), problem);
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QCOMPARE(plan->folder, QDir::toNativeSeparators(QDir(elsewhere.path()).filePath(QStringLiteral("Alpha - Classic"))));
    InstallJob job(*plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));
    QCOMPARE(QFileInfo(QDir(plan->folder).filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
    QVERIFY(QDir(stagingFolderFor(elsewhere.path())).exists());
    QCOMPARE(QDir(library.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot), QStringList());
  }

  void aPlanRefusesWithoutAPublishedClientOrRequiredTool()
  {
    FakeFileServer server;
    QString problem;
    QVERIFY(!planInstall(alpha(), {optionalPackage()}, QStringLiteral("D:/Games"), problem));
    QCOMPARE(problem, QStringLiteral("No client is published for Alpha Classic yet."));
    Package unpublishedClient = clientPackage(server);
    unpublishedClient.latest.reset();
    QVERIFY(!planInstall(alpha(), {unpublishedClient, toolPackage(server)}, QStringLiteral("D:/Games"), problem));
    QCOMPARE(problem, QStringLiteral("The client Alpha client has no version published yet."));
    Package unpublishedTool = toolPackage(server);
    unpublishedTool.latest.reset();
    QVERIFY(!planInstall(alpha(), {clientPackage(server), unpublishedTool}, QStringLiteral("D:/Games"), problem));
    QCOMPARE(problem, QStringLiteral("The required tool Alpha tools has no version published yet."));
  }

  void installsTheClientAndItsRequiredToolAndWritesTheReceipt()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    InstallJob job(planFor(server, library.path()), server.url(), QStringLiteral("secret-token"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));

    const QDir folder(QDir(library.path()).filePath(QStringLiteral("Alpha - Classic")));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Data/world.dat"))).size(), qint64(2400));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("readme.txt"))).size(), qint64(13));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("settings.ini"))).size(), qint64(20));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("tools/directConnect.dll"))).size(), qint64(1100));

    QString problem;
    const std::optional<Receipt> receipt = readReceipt(folder.absolutePath(), problem);
    QVERIFY2(receipt.has_value(), qPrintable(problem));
    QCOMPARE(receipt->client.name, QStringLiteral("alpha-client"));
    QCOMPARE(receipt->client.version, QStringLiteral("2026.11.01"));
    QCOMPARE(receipt->client.sha256, sha256Of(clientZip));
    QCOMPARE(receipt->tools.size(), 1);
    QCOMPARE(receipt->tools.first().version, QStringLiteral("2026.11.02-abc1234"));
    QCOMPARE(receipt->files.size(), 5);
    QCOMPARE(receipt->files.value(QStringLiteral("Data/world.dat")).package, QStringLiteral("alpha-client"));
    QCOMPARE(receipt->files.value(QStringLiteral("Data/world.dat")).crc32, 222u);
    QCOMPARE(receipt->files.value(QStringLiteral("settings.ini")).isWriteOnce, true);
    QCOMPARE(receipt->files.value(QStringLiteral("tools/directConnect.dll")).package, QStringLiteral("alpha-tools"));
    QCOMPARE(outcome.receipt.folder, QDir::toNativeSeparators(folder.absolutePath()));
    QCOMPARE(outcome.summary.written, 5);
    QCOMPARE(outcome.summary.deleted, 0);

    const QDir staging(stagingFolderFor(library.path()));
    QVERIFY(staging.exists());
    QCOMPARE(staging.entryList(QDir::AllEntries | QDir::NoDotAndDotDot), QStringList());

    QCOMPARE(outcome.statuses.first(), QStringLiteral("Reading the file list of Alpha client..."));
    QVERIFY(outcome.statuses.contains(QStringLiteral("Reading the file list of Alpha tools...")));
    QVERIFY(outcome.statuses.contains(QStringLiteral("Checking Alpha client...")));
    QVERIFY(outcome.statuses.contains(QStringLiteral("Extracting Alpha tools...")));
    QCOMPARE(outcome.statuses.last(), QStringLiteral("Moving the files into place..."));
    QCOMPARE(server.requests.size(), 4);
    for (const FakeFileServer::Recorded& request : server.requests) {
      QCOMPARE(request.authorization, QByteArray("Bearer secret-token"));
    }
    QCOMPARE(server.requests.at(0).path, QByteArray("/api/v1/packages/alpha-client/versions/2026.11.01/files"));
    QCOMPARE(server.requests.at(2).path, QByteArray("/files/client.zip"));
    QVERIFY(server.requests.at(2).range.isEmpty());
  }

  void resumesAPartialDownloadWithARangeRequest()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    QVERIFY(QDir().mkpath(stagingFolderFor(library.path())));
    writeFile(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part"), clientZip.left(300));
    InstallJob job(plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));
    QCOMPARE(server.requests.at(2).path, QByteArray("/files/client.zip"));
    QCOMPARE(server.requests.at(2).range, QByteArray("bytes=300-"));
    QCOMPARE(QFileInfo(QDir(library.path()).filePath(QStringLiteral("Alpha - Classic/Data/world.dat"))).size(), qint64(2400));
    QVERIFY(outcome.progress.contains(qMakePair(qint64(clientZip.size()), qint64(clientZip.size() + toolZip.size()))));
  }

  void aServerThatIgnoresRangeRestartsTheDownloadFromNothing()
  {
    FakeFileServer server;
    serveEverything(server);
    server.ignoresRange = true;
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    QVERIFY(QDir().mkpath(stagingFolderFor(library.path())));
    writeFile(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part"), QByteArray(300, 'x'));
    InstallJob job(plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));
    int clientRequests = 0;
    for (const FakeFileServer::Recorded& request : server.requests) {
      if (request.path == "/files/client.zip") {
        ++clientRequests;
        QCOMPARE(request.range, clientRequests == 1 ? QByteArray("bytes=300-") : QByteArray());
      }
    }
    QCOMPARE(clientRequests, 2);
    QCOMPARE(QFileInfo(QDir(library.path()).filePath(QStringLiteral("Alpha - Classic/Game.exe"))).size(), qint64(200));
  }

  void aDownloadWithTheWrongHashIsDeletedAndReported()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    InstallPlan plan = planFor(server, library.path());
    plan.client.latest->sha256 = QString(64, QLatin1Char('0'));
    plan.fetch.first().latest->sha256 = plan.client.latest->sha256;
    InstallJob job(plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY(outcome.isFailed);
    QCOMPARE(outcome.failure, QStringLiteral("The download of Alpha client does not match its published SHA-256, so it was deleted."));
    QVERIFY(!QFile::exists(stagedZipPath(library.path(), plan.client)));
    QVERIFY(!QFile::exists(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part")));
    QVERIFY(!QDir(plan.folder).exists());
  }

  void aMissingFileListFailsBeforeAnythingIsDownloaded()
  {
    FakeFileServer server;
    server.answer(QStringLiteral("/api/v1/packages/alpha-client/versions/2026.11.01/files"), 200, clientListing, "application/json");
    server.answer(QStringLiteral("/files/client.zip"), 200, clientZip, "application/octet-stream");
    QTemporaryDir library;
    InstallJob job(planFor(server, library.path()), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY(outcome.isFailed);
    QVERIFY2(outcome.failure.startsWith(QStringLiteral("Could not read the file list of Alpha tools: ")), qPrintable(outcome.failure));
    QCOMPARE(server.requests.size(), 2);
  }

  void aListingWithoutTheWriteOnceFlagIsRefused()
  {
    FakeFileServer server;
    serveEverything(server);
    server.answer(QStringLiteral("/api/v1/packages/alpha-client/versions/2026.11.01/files"), 200,
      "{\"files\":[{\"path\":\"Game.exe\",\"byteSize\":200,\"crc32\":111}]}", "application/json");
    QTemporaryDir library;
    InstallJob job(planFor(server, library.path()), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY(outcome.isFailed);
    QCOMPARE(outcome.failure, QStringLiteral("The server's answer was not understood: the file list of alpha-client has an entry that was not understood."));
  }

  void anExistingFolderIsNeverInstalledOver()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    QVERIFY(QDir().mkpath(plan.folder));
    InstallJob job(plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY(outcome.isFailed);
    QCOMPARE(outcome.failure, QStringLiteral("%1 already exists. Remove it before installing.").arg(plan.folder));
    QVERIFY(server.requests.isEmpty());
  }

  void cancellingMidDownloadKeepsThePartAndNothingElse()
  {
    FakeFileServer server;
    serveEverything(server);
    server.holdAfterBytes = 100;
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    InstallJob job(plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome, [&job](Outcome& soFar) {
      QTRY_VERIFY_WITH_TIMEOUT(!soFar.progress.isEmpty() && soFar.progress.last().first >= 100, 10000);
      job.cancel();
    });
    QVERIFY(outcome.isCancelled);
    QVERIFY(!outcome.isFailed && !outcome.isDone);
    QCOMPARE(QFileInfo(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part")).size(), qint64(100));
    QVERIFY(!QDir(plan.folder).exists());
    QCOMPARE(QDir(stagingFolderFor(library.path())).entryList(QDir::Dirs | QDir::NoDotAndDotDot), QStringList());
  }

  void bytesStillNeededSkipsWhatIsAlreadyStaged()
  {
    FakeFileServer server;
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    QCOMPARE(bytesStillNeeded(plan, library.path()), qint64(3733 + clientZip.size() + toolZip.size()));
    QVERIFY(QDir().mkpath(stagingFolderFor(library.path())));
    writeFile(stagedZipPath(library.path(), plan.client), clientZip);
    writeFile(stagedZipPath(library.path(), plan.tools.first()) + QStringLiteral(".part"), toolZip.left(39));
    QCOMPARE(bytesStillNeeded(plan, library.path()), qint64(3733 + toolZip.size() - 39));
  }

  void anUpdatePlanIsEmptyWhenEverythingMatchesTheReceipt()
  {
    FakeFileServer server;
    Receipt receipt{QStringLiteral("D:\\Games\\Alpha - Classic"), {QStringLiteral("alpha-client"), QStringLiteral("2026.11.01"), QStringLiteral("x")}, {}, {}};
    receipt.tools.append({QStringLiteral("alpha-tools"), QStringLiteral("2026.11.02-abc1234"), QStringLiteral("y")});
    QString problem = QStringLiteral("stale");
    QVERIFY(!planUpdate(alpha(), {clientPackage(server), toolPackage(server), optionalPackage()}, receipt, problem));
    QVERIFY(problem.isEmpty());
  }

  void anUpdatePlanFetchesChangedAndNewPackagesAndRemovesDetachedOnes()
  {
    FakeFileServer server;
    Receipt receipt{QStringLiteral("D:\\Games\\Alpha - Classic"), {QStringLiteral("alpha-client"), QStringLiteral("2026.11.01"), QStringLiteral("x")}, {}, {}};
    receipt.tools.append({QStringLiteral("old-tool"), QStringLiteral("1"), QStringLiteral("z")});
    QString problem;
    const std::optional<InstallPlan> plan = planUpdate(alpha(), {client2Package(server), toolPackage(server)}, receipt, problem);
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QVERIFY(plan->isUpdate());
    QCOMPARE(plan->folder, receipt.folder);
    QCOMPARE(plan->fetch.size(), 2);
    QCOMPARE(plan->fetch.at(0).name, QStringLiteral("alpha-client"));
    QCOMPARE(plan->fetch.at(1).name, QStringLiteral("alpha-tools"));
    QCOMPARE(plan->tools.size(), 1);
    QCOMPARE(plan->removed, QStringList{QStringLiteral("old-tool")});
    QCOMPARE(plan->downloadBytes(), qint64(client2Zip.size() + toolZip.size()));

    Receipt otherClient = receipt;
    otherClient.client.name = QStringLiteral("other-client");
    QVERIFY(!planUpdate(alpha(), {client2Package(server), toolPackage(server)}, otherClient, problem));
    QCOMPARE(problem, QStringLiteral("The edition's client changed from other-client to alpha-client. Remove D:\\Games\\Alpha - Classic and install again."));
  }

  void anUpdateReplacesChangedFilesLeavesUnchangedOnesDeletesDroppedOnesAndKeepsWriteOnceEdits()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const Receipt first = installFirstVersion(server, library.path());
    const QDir folder(first.folder);
    writeFile(folder.filePath(QStringLiteral("settings.ini")), "[video]\r\nwidth=1920\r\n");
    writeFile(folder.filePath(QStringLiteral("Data/world.dat")), "edited by the player");
    writeFile(folder.filePath(QStringLiteral("Screenshots/shot.png")), "png");
    writeFile(folder.filePath(QStringLiteral("Game.exe")), "patched by the player");

    QString problem;
    const std::optional<InstallPlan> plan = planUpdate(alpha(), {client2Package(server), toolPackage(server), optionalPackage()}, first, problem);
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QCOMPARE(plan->fetch.size(), 1);
    InstallJob job(*plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));
    QCOMPARE(outcome.statuses.last(), QStringLiteral("Applying the update..."));

    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Game.exe"))).size(), qint64(300));
    QVERIFY(!QFile::exists(folder.filePath(QStringLiteral("readme.txt"))));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Data/extra.dat"))).size(), qint64(500));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("Data/world.dat"))), QByteArray("edited by the player"));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("settings.ini"))), QByteArray("[video]\r\nwidth=1920\r\n"));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("Screenshots/shot.png"))), QByteArray("png"));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("tools/directConnect.dll"))).size(), qint64(1100));
    QCOMPARE(outcome.summary.written, 2);
    QCOMPARE(outcome.summary.deleted, 1);
    QCOMPARE(outcome.summary.kept, QStringList{QStringLiteral("settings.ini")});

    QCOMPARE(outcome.receipt.client.version, QStringLiteral("2026.11.09"));
    QCOMPARE(outcome.receipt.client.sha256, sha256Of(client2Zip));
    QCOMPARE(outcome.receipt.tools.size(), 1);
    QCOMPARE(outcome.receipt.tools.first().version, QStringLiteral("2026.11.02-abc1234"));
    QCOMPARE(outcome.receipt.files.size(), 5);
    QVERIFY(!outcome.receipt.files.contains(QStringLiteral("readme.txt")));
    QCOMPARE(outcome.receipt.files.value(QStringLiteral("settings.ini")).crc32, 777u);
    QCOMPARE(outcome.receipt.files.value(QStringLiteral("Data/extra.dat")).package, QStringLiteral("alpha-client"));
    const std::optional<Receipt> stored = readReceipt(first.folder, problem);
    QVERIFY2(stored.has_value(), qPrintable(problem));
    QCOMPARE(stored->client.version, QStringLiteral("2026.11.09"));
    QCOMPARE(QDir(stagingFolderFor(library.path())).entryList(QDir::AllEntries | QDir::NoDotAndDotDot), QStringList());
    QVERIFY(!planUpdate(alpha(), {client2Package(server), toolPackage(server), optionalPackage()}, *stored, problem));
    QVERIFY(problem.isEmpty());
  }

  void anInstallTakesTheOptionalToolsItIsToldToAndRefusesUnknownOnes()
  {
    FakeFileServer server;
    QString problem;
    const QList<Package> packages = {clientPackage(server), toolPackage(server), mapsPackage(server)};
    const std::optional<InstallPlan> plain = planInstall(alpha(), packages, QStringLiteral("D:/Games"), problem);
    QVERIFY2(plain.has_value(), qPrintable(problem));
    QCOMPARE(plain->fetch.size(), 2);
    const std::optional<InstallPlan> withMaps = planInstall(alpha(), packages, QStringLiteral("D:/Games"), problem, {QStringLiteral("alpha-maps")});
    QVERIFY2(withMaps.has_value(), qPrintable(problem));
    QCOMPARE(withMaps->fetch.size(), 3);
    QCOMPARE(withMaps->tools.last().name, QStringLiteral("alpha-maps"));
    QVERIFY(!planInstall(alpha(), packages, QStringLiteral("D:/Games"), problem, {QStringLiteral("alpha-tools")}));
    QCOMPARE(problem, QStringLiteral("alpha-tools is not an optional tool of Alpha Classic."));
    QVERIFY(!planInstall(alpha(), packages, QStringLiteral("D:/Games"), problem, {QStringLiteral("nothing")}));
    Package unpublished = mapsPackage(server);
    unpublished.latest.reset();
    QVERIFY(!planInstall(alpha(), {clientPackage(server), toolPackage(server), unpublished}, QStringLiteral("D:/Games"), problem, {QStringLiteral("alpha-maps")}));
    QCOMPARE(problem, QStringLiteral("The optional tool Alpha maps has no version published yet."));
  }

  void aToolChangeAddsAnOptionalToolThenRemovesItKeepingItsWriteOnceFiles()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const Receipt first = installFirstVersion(server, library.path());
    const QList<Package> packages = {clientPackage(server), toolPackage(server), mapsPackage(server)};
    QString problem = QStringLiteral("stale");
    QVERIFY(!planToolChange(alpha(), packages, first, {}, problem));
    QVERIFY(problem.isEmpty());

    const std::optional<InstallPlan> add = planToolChange(alpha(), packages, first, {QStringLiteral("alpha-maps")}, problem);
    QVERIFY2(add.has_value(), qPrintable(problem));
    QVERIFY(add->isToolChange && add->isUpdate());
    QCOMPARE(add->fetch.size(), 1);
    QCOMPARE(add->fetch.first().name, QStringLiteral("alpha-maps"));
    QVERIFY(add->removed.isEmpty());
    QCOMPARE(add->tools.size(), 2);
    InstallJob addJob(*add, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome added;
    run(addJob, added);
    QVERIFY2(added.isDone, qPrintable(added.failure));
    const QDir folder(first.folder);
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("maps/world.map"))).size(), qint64(400));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
    QCOMPARE(added.summary.written, 2);
    QCOMPARE(added.receipt.client.version, QStringLiteral("2026.11.01"));
    QCOMPARE(added.receipt.tools.size(), 2);
    QCOMPARE(added.receipt.files.size(), 7);
    QVERIFY(added.receipt.files.value(QStringLiteral("maps/maps.ini")).isWriteOnce);
    QVERIFY(!planToolChange(alpha(), packages, added.receipt, {QStringLiteral("alpha-maps")}, problem));

    writeFile(folder.filePath(QStringLiteral("maps/maps.ini")), "zoom=3\r\n");
    const std::optional<InstallPlan> remove = planToolChange(alpha(), packages, added.receipt, {}, problem);
    QVERIFY2(remove.has_value(), qPrintable(problem));
    QVERIFY(remove->fetch.isEmpty());
    QCOMPARE(remove->removed, QStringList{QStringLiteral("alpha-maps")});
    const int requestsBefore = server.requests.size();
    InstallJob removeJob(*remove, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome removed;
    run(removeJob, removed);
    QVERIFY2(removed.isDone, qPrintable(removed.failure));
    QCOMPARE(server.requests.size(), requestsBefore);
    QVERIFY(!QFile::exists(folder.filePath(QStringLiteral("maps/world.map"))));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("maps/maps.ini"))), QByteArray("zoom=3\r\n"));
    QCOMPARE(removed.summary.deleted, 1);
    QCOMPARE(removed.summary.kept, QStringList{QStringLiteral("maps/maps.ini")});
    QCOMPARE(removed.receipt.tools.size(), 1);
    QCOMPARE(removed.receipt.tools.first().name, QStringLiteral("alpha-tools"));
    QCOMPARE(removed.receipt.files.size(), 5);
    QVERIFY(!removed.receipt.files.contains(QStringLiteral("maps/maps.ini")));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
  }

  void aToolChangeLeavesOutdatedPackagesAloneAndFetchesAMissingRequiredTool()
  {
    FakeFileServer server;
    Receipt receipt{QStringLiteral("D:\\Games\\Alpha - Classic"), {QStringLiteral("alpha-client"), QStringLiteral("2026.11.01"), QStringLiteral("x")}, {}, {}};
    QString problem;
    const std::optional<InstallPlan> plan = planToolChange(alpha(), {client2Package(server), toolPackage(server), mapsPackage(server)}, receipt,
      {QStringLiteral("alpha-maps")}, problem);
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QStringList fetched;
    for (const Package& package : plan->fetch) {
      fetched.append(package.name);
    }
    QCOMPARE(fetched, (QStringList{QStringLiteral("alpha-tools"), QStringLiteral("alpha-maps")}));
    QVERIFY(!planToolChange(alpha(), {client2Package(server), toolPackage(server)}, receipt, {QStringLiteral("unknown")}, problem));
    QCOMPARE(problem, QStringLiteral("unknown is not a tool of Alpha Classic."));
  }

  void toolsAreAttachedToThePlayersOwnCopyUpdatedAndRemovedWithoutTouchingTheGame()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir parent;
    const QDir folder(QDir(parent.path()).filePath(QStringLiteral("My Alpha")));
    const QList<Package> packages = {toolPackage(server), mapsPackage(server)};
    QString problem;
    QVERIFY(folder.mkpath(QStringLiteral(".")));
    QVERIFY(!planAttach(ownCopy(), packages, folder.path(), problem));
    QVERIFY2(problem.contains(QStringLiteral("is not a copy of Alpha: it has no bin\\alpha.exe")), qPrintable(problem));

    writeFile(folder.filePath(QStringLiteral("bin/alpha.exe")), "the game itself");
    writeFile(folder.filePath(QStringLiteral("data/world.dat")), "the game's data");
    writeFile(folder.filePath(QStringLiteral("tools/directConnect.dll")), "a copy put there by hand");
    const std::optional<InstallPlan> plan = planAttach(ownCopy(), packages, folder.path(), problem, {QStringLiteral("alpha-maps")});
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QVERIFY(plan->isAttach && plan->isUpdate());
    QCOMPARE(plan->folder, QDir::toNativeSeparators(folder.absolutePath()));
    QCOMPARE(plan->fetch.size(), 2);
    QCOMPARE(stagingRootFor(*plan), QDir::toNativeSeparators(folder.filePath(QStringLiteral(".ankh"))));

    InstallJob job(*plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome attached;
    run(job, attached);
    QVERIFY2(attached.isDone, qPrintable(attached.failure));
    QCOMPARE(attached.statuses.last(), QStringLiteral("Adding the tools..."));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("bin/alpha.exe"))), QByteArray("the game itself"));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("data/world.dat"))), QByteArray("the game's data"));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("tools/directConnect.dll"))).size(), qint64(1100));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("maps/world.map"))).size(), qint64(400));
    QCOMPARE(attached.summary.written, 3);
    QCOMPARE(attached.summary.replaced, QStringList{QStringLiteral("tools/directConnect.dll")});
    QVERIFY(attached.receipt.isAttached());
    QCOMPARE(attached.receipt.attachedGame, QStringLiteral("Alpha"));
    QCOMPARE(attached.receipt.tools.size(), 2);
    QCOMPARE(attached.receipt.files.size(), 3);
    QVERIFY(!attached.receipt.files.contains(QStringLiteral("bin/alpha.exe")));
    QVERIFY(QDir(parent.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot) == QStringList{QStringLiteral("My Alpha")});
    const std::optional<Receipt> stored = readReceipt(folder.path(), problem);
    QVERIFY2(stored.has_value(), qPrintable(problem));
    QVERIFY(stored->isAttached());
    QVERIFY(!planUpdate(ownCopy(), packages, *stored, problem));
    QVERIFY(problem.isEmpty());

    Package newerTool = toolPackage(server);
    newerTool.latest->version = QStringLiteral("2026.11.20-def5678");
    server.answer(QStringLiteral("/api/v1/packages/alpha-tools/versions/2026.11.20-def5678/files"), 200, toolListing, "application/json");
    const std::optional<InstallPlan> update = planUpdate(ownCopy(), {newerTool, mapsPackage(server)}, *stored, problem);
    QVERIFY2(update.has_value(), qPrintable(problem));
    QCOMPARE(update->fetch.size(), 1);
    QCOMPARE(update->fetch.first().name, QStringLiteral("alpha-tools"));
    InstallJob updateJob(*update, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome updated;
    run(updateJob, updated);
    QVERIFY2(updated.isDone, qPrintable(updated.failure));
    QVERIFY(updated.receipt.isAttached());
    QCOMPARE(updated.receipt.tools.first().version, QStringLiteral("2026.11.20-def5678"));
    QCOMPARE(updated.receipt.tools.size(), 2);

    const std::optional<InstallPlan> remove = planToolChange(ownCopy(), packages, updated.receipt, {}, problem);
    QVERIFY2(remove.has_value(), qPrintable(problem));
    QCOMPARE(remove->removed, QStringList{QStringLiteral("alpha-maps")});
    InstallJob removeJob(*remove, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome removed;
    run(removeJob, removed);
    QVERIFY2(removed.isDone, qPrintable(removed.failure));
    QVERIFY(!QFile::exists(folder.filePath(QStringLiteral("maps/world.map"))));
    QVERIFY(QFile::exists(folder.filePath(QStringLiteral("maps/maps.ini"))));
    QCOMPARE(contentOf(folder.filePath(QStringLiteral("bin/alpha.exe"))), QByteArray("the game itself"));
    QVERIFY(removed.receipt.isAttached());
    QCOMPARE(removed.receipt.tools.size(), 1);
  }

  void anAttachNeedsAPublishedRequiredToolAndAMuseumReceiptIsNotAnAttachedOne()
  {
    FakeFileServer server;
    QTemporaryDir folder;
    writeFile(QDir(folder.path()).filePath(QStringLiteral("bin/alpha.exe")), "the game itself");
    QString problem;
    QVERIFY(!planAttach(ownCopy(), {}, folder.path(), problem));
    QCOMPARE(problem, QStringLiteral("No tools are published for Alpha Classic yet."));
    Package unpublished = toolPackage(server);
    unpublished.latest.reset();
    QVERIFY(!planAttach(ownCopy(), {unpublished}, folder.path(), problem));
    QCOMPARE(problem, QStringLiteral("The required tool Alpha tools has no version published yet."));

    Receipt museum{QStringLiteral("D:\\Games\\Alpha - Classic"), {QStringLiteral("alpha-client"), QStringLiteral("2026.11.01"), QStringLiteral("x")}, {}, {}};
    QVERIFY(!planUpdate(ownCopy(), {toolPackage(server)}, museum, problem));
    QVERIFY2(problem.contains(QStringLiteral("holds a different kind of install")), qPrintable(problem));
    QVERIFY(!planToolChange(ownCopy(), {toolPackage(server)}, museum, {}, problem));
    QVERIFY(!problem.isEmpty());
  }

  void anUpdateRemovesAToolThatLeftTheEdition()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const Receipt first = installFirstVersion(server, library.path());
    QString problem;
    const std::optional<InstallPlan> plan = planUpdate(alpha(), {clientPackage(server), optionalPackage()}, first, problem);
    QVERIFY2(plan.has_value(), qPrintable(problem));
    QVERIFY(plan->fetch.isEmpty());
    QCOMPARE(plan->removed, QStringList{QStringLiteral("alpha-tools")});
    const int requestsBefore = server.requests.size();
    InstallJob job(*plan, server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));
    QCOMPARE(server.requests.size(), requestsBefore);
    QVERIFY(!QFile::exists(QDir(first.folder).filePath(QStringLiteral("tools/directConnect.dll"))));
    QCOMPARE(QFileInfo(QDir(first.folder).filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
    QCOMPARE(outcome.summary.deleted, 1);
    QCOMPARE(outcome.summary.written, 0);
    QVERIFY(outcome.receipt.tools.isEmpty());
    QCOMPARE(outcome.receipt.files.size(), 4);
    QCOMPARE(outcome.receipt.client.version, QStringLiteral("2026.11.01"));
  }
};

QTEST_GUILESS_MAIN(InstallJobTest)
#include "installJobTest.moc"
