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
  const QByteArray toolZip = fixture("tool.zip");
  const QByteArray clientListing = "{\"files\":[{\"path\":\"Game.exe\",\"byteSize\":200,\"crc32\":111},"
    "{\"path\":\"Data/world.dat\",\"byteSize\":2400,\"crc32\":222},{\"path\":\"readme.txt\",\"byteSize\":13,\"crc32\":333}]}";
  const QByteArray toolListing = "{\"files\":[{\"path\":\"tools/directConnect.dll\",\"byteSize\":1100,\"crc32\":444}]}";

  struct Outcome {
    QStringList statuses;
    QList<QPair<qint64, qint64>> progress;
    bool isDone = false;
    bool isFailed = false;
    bool isCancelled = false;
    QString failure;
    Receipt receipt;
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
        QStringLiteral("2026-11-01T00:00:00Z"), 3, 2613, QUrl(server.url().toString() + QStringLiteral("/files/client.zip"))}};
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

  void serveEverything(FakeFileServer& server)
  {
    server.answer(QStringLiteral("/api/v1/packages/alpha-client/versions/2026.11.01/files"), 200, clientListing, "application/json");
    server.answer(QStringLiteral("/api/v1/packages/alpha-tools/versions/2026.11.02-abc1234/files"), 200, toolListing, "application/json");
    server.answer(QStringLiteral("/files/client.zip"), 200, clientZip, "application/octet-stream");
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
    connect(&job, &InstallJob::finished, this, [&outcome](const Receipt& receipt) {
      outcome.receipt = receipt;
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

private slots:
  void aPlanTakesTheClientAndRequiredToolsAndNamesTheFolder()
  {
    FakeFileServer server;
    const InstallPlan plan = planFor(server, QStringLiteral("D:/Games/clients"));
    QCOMPARE(plan.client.name, QStringLiteral("alpha-client"));
    QCOMPARE(plan.tools.size(), 1);
    QCOMPARE(plan.tools.first().name, QStringLiteral("alpha-tools"));
    QCOMPARE(plan.folder, QStringLiteral("D:\\Games\\clients\\Alpha - Classic"));
    QCOMPARE(plan.downloadBytes(), qint64(clientZip.size() + toolZip.size()));
    QCOMPARE(plan.extractedBytes(), qint64(3713));
    QCOMPARE(plan.fileCount(), 4);
    QCOMPARE(stagedZipPath(QStringLiteral("D:/Games/clients"), plan.client), QStringLiteral("D:\\Games\\clients\\.staging\\alpha-client-2026.11.01.zip"));
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
    InstallJob job(planFor(server, library.path()), library.path(), server.url(), QStringLiteral("secret-token"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY2(outcome.isDone, qPrintable(outcome.failure));

    const QDir folder(QDir(library.path()).filePath(QStringLiteral("Alpha - Classic")));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Game.exe"))).size(), qint64(200));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("Data/world.dat"))).size(), qint64(2400));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("readme.txt"))).size(), qint64(13));
    QCOMPARE(QFileInfo(folder.filePath(QStringLiteral("tools/directConnect.dll"))).size(), qint64(1100));

    QString problem;
    const std::optional<Receipt> receipt = readReceipt(folder.absolutePath(), problem);
    QVERIFY2(receipt.has_value(), qPrintable(problem));
    QCOMPARE(receipt->client.name, QStringLiteral("alpha-client"));
    QCOMPARE(receipt->client.version, QStringLiteral("2026.11.01"));
    QCOMPARE(receipt->client.sha256, sha256Of(clientZip));
    QCOMPARE(receipt->tools.size(), 1);
    QCOMPARE(receipt->tools.first().version, QStringLiteral("2026.11.02-abc1234"));
    QCOMPARE(receipt->files.size(), 4);
    QCOMPARE(receipt->files.value(QStringLiteral("Data/world.dat")).package, QStringLiteral("alpha-client"));
    QCOMPARE(receipt->files.value(QStringLiteral("Data/world.dat")).crc32, 222u);
    QCOMPARE(receipt->files.value(QStringLiteral("tools/directConnect.dll")).package, QStringLiteral("alpha-tools"));
    QCOMPARE(outcome.receipt.folder, QDir::toNativeSeparators(folder.absolutePath()));

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
    QFile part(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part"));
    QVERIFY(part.open(QIODevice::WriteOnly));
    part.write(clientZip.left(300));
    part.close();
    InstallJob job(plan, library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
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
    QFile part(stagedZipPath(library.path(), plan.client) + QStringLiteral(".part"));
    QVERIFY(part.open(QIODevice::WriteOnly));
    part.write(QByteArray(300, 'x'));
    part.close();
    InstallJob job(plan, library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
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
    InstallJob job(plan, library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
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
    InstallJob job(planFor(server, library.path()), library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
    Outcome outcome;
    run(job, outcome);
    QVERIFY(outcome.isFailed);
    QVERIFY2(outcome.failure.startsWith(QStringLiteral("Could not read the file list of Alpha tools: ")), qPrintable(outcome.failure));
    QCOMPARE(server.requests.size(), 2);
  }

  void anExistingFolderIsNeverInstalledOver()
  {
    FakeFileServer server;
    serveEverything(server);
    QTemporaryDir library;
    const InstallPlan plan = planFor(server, library.path());
    QVERIFY(QDir().mkpath(plan.folder));
    InstallJob job(plan, library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
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
    InstallJob job(plan, library.path(), server.url(), QStringLiteral("t"), QStringLiteral("2026.11.08.1200"));
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
    QCOMPARE(bytesStillNeeded(plan, library.path()), qint64(3713 + clientZip.size() + toolZip.size()));
    QVERIFY(QDir().mkpath(stagingFolderFor(library.path())));
    QFile staged(stagedZipPath(library.path(), plan.client));
    QVERIFY(staged.open(QIODevice::WriteOnly));
    staged.write(clientZip);
    staged.close();
    QFile part(stagedZipPath(library.path(), plan.tools.first()) + QStringLiteral(".part"));
    QVERIFY(part.open(QIODevice::WriteOnly));
    part.write(toolZip.left(39));
    part.close();
    QCOMPARE(bytesStillNeeded(plan, library.path()), qint64(3713 + toolZip.size() - 39));
  }
};

QTEST_GUILESS_MAIN(InstallJobTest)
#include "installJobTest.moc"
