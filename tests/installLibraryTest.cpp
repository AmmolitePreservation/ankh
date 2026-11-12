#include "installLibrary.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

namespace {

Package clientFor(const QList<Edition>& editions)
{
  return {QStringLiteral("wow-classic"), QStringLiteral("Classic client"), QStringLiteral("client"), editions, std::nullopt};
}

Receipt sampleReceipt(const QString& folder)
{
  Receipt receipt{folder, {QStringLiteral("wow-classic"), QStringLiteral("2026.10.13"), QStringLiteral("d298")}, {}, {}};
  receipt.plugins.append({QStringLiteral("wow-login"), QStringLiteral("2026.11.01-abc1234"), QStringLiteral("5580")});
  receipt.files.insert(QStringLiteral("WoW.exe"), {QStringLiteral("wow-classic"), 5335826980, 0xDEADBEEFu, false});
  receipt.files.insert(QStringLiteral("Data/common.MPQ"), {QStringLiteral("wow-classic"), 12, 7u, false});
  receipt.files.insert(QStringLiteral("WTF/Config.wtf"), {QStringLiteral("wow-classic"), 40, 8u, true});
  receipt.files.insert(QStringLiteral("ankh-plugins/login.dll"), {QStringLiteral("wow-login"), 3, 9u, false});
  return receipt;
}

}

class InstallLibraryTest : public QObject {
  Q_OBJECT

private slots:
  void aClientOfOneEditionIsNamedForTheGameAndEdition()
  {
    QCOMPARE(installFolderNameFor(clientFor({{QStringLiteral("World of Warcraft"), QStringLiteral("classic"), QStringLiteral("Classic (1.12.1)")}})),
      QStringLiteral("World of Warcraft - Classic (1.12.1)"));
  }

  void aClientServingSeveralEditionsIsNamedForTheGameAlone()
  {
    QCOMPARE(installFolderNameFor(clientFor({
      {QStringLiteral("EverQuest"), QStringLiteral("classic"), QStringLiteral("Classic")},
      {QStringLiteral("EverQuest"), QStringLiteral("pop"), QStringLiteral("Planes of Power")}})), QStringLiteral("EverQuest"));
    QCOMPARE(installFolderNameFor(clientFor({})), QString());
  }

  void charactersWindowsRefusesInAFolderNameAreReplaced()
  {
    QCOMPARE(installFolderNameFor(clientFor({{QStringLiteral("Asheron's Call"), QStringLiteral("end"), QStringLiteral("End of Retail: 2017?")}})),
      QStringLiteral("Asheron's Call - End of Retail- 2017-"));
    QCOMPARE(installFolderNameFor(clientFor({{QStringLiteral("Game"), QStringLiteral("v"), QStringLiteral("Version 1.0.")}})),
      QStringLiteral("Game - Version 1.0"));
  }

  void aReceiptIsWrittenAndReadBackWhole()
  {
    QTemporaryDir library;
    const QString folder = QDir(library.path()).filePath(QStringLiteral("World of Warcraft - Classic (1.12.1)"));
    QVERIFY(QDir().mkpath(folder));
    const Receipt written = sampleReceipt(QDir::toNativeSeparators(folder));
    QCOMPARE(writeReceipt(written), QString());
    QVERIFY(QFile::exists(QDir(folder).filePath(QStringLiteral(".ankh/receipt.json"))));
    QString problem;
    const std::optional<Receipt> read = readReceipt(folder, problem);
    QVERIFY2(read.has_value(), qPrintable(problem));
    QCOMPARE(read->folder, written.folder);
    QCOMPARE(read->client.name, QStringLiteral("wow-classic"));
    QCOMPARE(read->client.version, QStringLiteral("2026.10.13"));
    QCOMPARE(read->client.sha256, QStringLiteral("d298"));
    QCOMPARE(read->plugins.size(), 1);
    QCOMPARE(read->plugins.first().name, QStringLiteral("wow-login"));
    QCOMPARE(read->files.size(), 4);
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).byteSize, qint64(5335826980));
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).crc32, 0xDEADBEEFu);
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).isWriteOnce, false);
    QCOMPARE(read->files.value(QStringLiteral("WTF/Config.wtf")).isWriteOnce, true);
    QCOMPARE(read->files.value(QStringLiteral("ankh-plugins/login.dll")).package, QStringLiteral("wow-login"));
  }

  void anAttachedReceiptHasNoClientAndIsFoundByItsGame()
  {
    QTemporaryDir folder;
    Receipt receipt{QDir::toNativeSeparators(folder.path()), {}, {}, {}, QStringLiteral("Dungeons & Dragons Online"), QStringLiteral("live")};
    receipt.plugins.append({QStringLiteral("ddo-plugins"), QStringLiteral("2026.11.09"), QStringLiteral("aa11")});
    receipt.files.insert(QStringLiteral("x64/version.dll"), {QStringLiteral("ddo-plugins"), 10, 5u, false});
    QCOMPARE(writeReceipt(receipt), QString());
    QFile stored(receiptPath(folder.path()));
    QVERIFY(stored.open(QIODevice::ReadOnly));
    const QByteArray text = stored.readAll();
    QVERIFY(text.contains("\"client\": null"));
    QVERIFY(text.contains("\"game\": \"Dungeons & Dragons Online\""));
    stored.close();

    QString problem;
    const std::optional<Receipt> read = readReceipt(folder.path(), problem);
    QVERIFY2(read.has_value(), qPrintable(problem));
    QVERIFY(read->isAttached());
    QCOMPARE(read->attachedGame, QStringLiteral("Dungeons & Dragons Online"));
    QCOMPARE(read->attachedVariant, QStringLiteral("live"));
    QCOMPARE(read->client.name, QString());
    QCOMPARE(read->plugins.size(), 1);

    QTemporaryDir museumFolder;
    const Receipt museum = sampleReceipt(QDir::toNativeSeparators(museumFolder.path()));
    QVERIFY(!museum.isAttached());
    const QList<Receipt> receipts = {museum, *read};
    QCOMPARE(receiptForAttached(receipts, QStringLiteral("Dungeons & Dragons Online"), QStringLiteral("live"))->folder, read->folder);
    QVERIFY(receiptForAttached(receipts, QStringLiteral("Dungeons & Dragons Online"), QStringLiteral("other")) == nullptr);
    QCOMPARE(receiptForClient(receipts, QStringLiteral("wow-classic"))->folder, museum.folder);
    QVERIFY(receiptForClient(receipts, QString()) == nullptr);
  }

  void aReceiptFromAnEarlierReleaseKeepsItsInstallAndIsRewrittenWithTodaysField()
  {
    QTemporaryDir library;
    const QString folder = QDir(library.path()).filePath(QStringLiteral("Earlier"));
    QVERIFY(QDir().mkpath(folder + QStringLiteral("/.ankh")));
    QFile earlier(receiptPath(folder));
    QVERIFY(earlier.open(QIODevice::WriteOnly));
    earlier.write("{\"client\": {\"name\": \"alpha-client\", \"version\": \"1\", \"sha256\": \"aa\"}, "
      "\"tools\": [{\"name\": \"alpha-fixes\", \"version\": \"2\", \"sha256\": \"bb\"}], "
      "\"files\": {\"fixes.dll\": {\"package\": \"alpha-fixes\", \"byteSize\": 3, \"crc32\": 9, \"writeOnce\": false}}}");
    earlier.close();
    QString problem;
    const std::optional<Receipt> read = readReceipt(folder, problem);
    QVERIFY2(read.has_value(), qPrintable(problem));
    QCOMPARE(read->client.name, QStringLiteral("alpha-client"));
    QCOMPARE(read->plugins.size(), 1);
    QCOMPARE(read->plugins.first().name, QStringLiteral("alpha-fixes"));
    QCOMPARE(read->plugins.first().version, QStringLiteral("2"));
    QCOMPARE(read->files.value(QStringLiteral("fixes.dll")).package, QStringLiteral("alpha-fixes"));

    QFile rewritten(receiptPath(folder));
    QVERIFY(rewritten.open(QIODevice::ReadOnly));
    const QJsonObject object = QJsonDocument::fromJson(rewritten.readAll()).object();
    QVERIFY(!object.contains(QStringLiteral("tools")));
    QCOMPARE(object.value(QStringLiteral("plugins")).toArray().size(), 1);
    QCOMPARE(object.value(QStringLiteral("plugins")).toArray().first().toObject().value(QStringLiteral("name")).toString(),
      QStringLiteral("alpha-fixes"));
    const std::optional<Receipt> again = readReceipt(folder, problem);
    QVERIFY2(again.has_value(), qPrintable(problem));
    QCOMPARE(again->plugins.size(), 1);
  }

  void aReceiptWithNoClientMustSayWhichGameItIsAttachedTo()
  {
    QTemporaryDir folder;
    QVERIFY(QDir(folder.path()).mkpath(QStringLiteral(".ankh")));
    QFile file(receiptPath(folder.path()));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("{\"client\": null, \"plugins\": [], \"files\": {}}");
    file.close();
    QString problem;
    QVERIFY(!readReceipt(folder.path(), problem).has_value());
    QVERIFY2(problem.contains(QStringLiteral("the field \"attached\" is missing")), qPrintable(problem));
  }

  void aReceiptWithoutTheWriteOnceFlagIsRefused()
  {
    QTemporaryDir library;
    const QString folder = QDir(library.path()).filePath(QStringLiteral("Old"));
    QVERIFY(QDir().mkpath(folder + QStringLiteral("/.ankh")));
    QFile old(QDir(folder).filePath(QStringLiteral(".ankh/receipt.json")));
    QVERIFY(old.open(QIODevice::WriteOnly));
    old.write("{\"client\": {\"name\": \"a\", \"version\": \"1\", \"sha256\": \"b\"}, \"plugins\": [], "
      "\"files\": {\"a.exe\": {\"package\": \"a\", \"byteSize\": 1, \"crc32\": 2}}}");
    old.close();
    QString problem;
    QVERIFY(!readReceipt(folder, problem).has_value());
    QVERIFY2(problem.endsWith(QStringLiteral("is not a valid install record: the field \"writeOnce\" is missing.")), qPrintable(problem));
  }

  void aScanFindsEveryReceiptAndNamesTheBrokenOnes()
  {
    QTemporaryDir library;
    const QDir root(library.path());
    QVERIFY(root.mkpath(QStringLiteral("World of Warcraft - Classic (1.12.1)")));
    QCOMPARE(writeReceipt(sampleReceipt(root.filePath(QStringLiteral("World of Warcraft - Classic (1.12.1)")))), QString());
    QVERIFY(root.mkpath(QStringLiteral("EverQuest/.ankh")));
    QFile broken(root.filePath(QStringLiteral("EverQuest/.ankh/receipt.json")));
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("{\"client\": {\"name\": \"everquest\"}, \"plugins\": [], \"files\": {}}");
    broken.close();
    QVERIFY(root.mkpath(QStringLiteral("Not an install")));
    QVERIFY(root.mkpath(QStringLiteral(".staging")));
    const LibraryScan scan = scanLibrary(library.path(), {});
    QCOMPARE(scan.receipts.size(), 1);
    QCOMPARE(scan.receipts.first().client.name, QStringLiteral("wow-classic"));
    QCOMPARE(scan.problems.size(), 1);
    QVERIFY2(scan.problems.first().contains(QStringLiteral("receipt.json is not a valid install record: the field \"version\" is missing")), qPrintable(scan.problems.first()));
    QVERIFY(receiptForClient(scan.receipts, QStringLiteral("wow-classic")) != nullptr);
    QVERIFY(receiptForClient(scan.receipts, QStringLiteral("everquest")) == nullptr);
  }

  void onlyARootedPathIsALibraryFolder()
  {
    QCOMPARE(libraryRootProblem(QStringLiteral("C:\\Games\\Ankh")), QString());
    QCOMPARE(libraryRootProblem(QStringLiteral("D:/clients")), QString());
    QCOMPARE(libraryRootProblem(QStringLiteral("\\\\zircon\\Media\\ankh")), QString());
    QVERIFY(libraryRootProblem(QStringLiteral("C:amesnkh")).startsWith(QStringLiteral("The library folder C:amesnkh is not an absolute path")));
    QVERIFY(libraryRootProblem(QStringLiteral("clients")).startsWith(QStringLiteral("The library folder clients is not an absolute path")));
    QVERIFY(libraryRootProblem(QStringLiteral("\\clients")).contains(QStringLiteral("not an absolute path")));
    QCOMPARE(libraryRootProblem(QStringLiteral("  ")), QStringLiteral("The library folder setting is empty."));
  }

  void aMissingLibraryScansEmpty()
  {
    const LibraryScan scan = scanLibrary(QStringLiteral("C:/definitely/not/here/ankh"), {});
    QVERIFY(scan.receipts.isEmpty() && scan.problems.isEmpty());
  }

  void rememberedInstallsOutsideTheLibraryAreFoundOnce()
  {
    QTemporaryDir library;
    QTemporaryDir elsewhere;
    const QString inLibrary = QDir(library.path()).filePath(QStringLiteral("World of Warcraft - Classic (1.12.1)"));
    const QString outside = QDir(elsewhere.path()).filePath(QStringLiteral("EverQuest"));
    const QString broken = QDir(elsewhere.path()).filePath(QStringLiteral("Broken"));
    QVERIFY(QDir().mkpath(inLibrary) && QDir().mkpath(outside) && QDir().mkpath(broken + QStringLiteral("/.ankh")));
    QCOMPARE(writeReceipt(sampleReceipt(inLibrary)), QString());
    Receipt everquest = sampleReceipt(outside);
    everquest.client.name = QStringLiteral("everquest");
    QCOMPARE(writeReceipt(everquest), QString());
    QFile brokenReceipt(QDir(broken).filePath(QStringLiteral(".ankh/receipt.json")));
    QVERIFY(brokenReceipt.open(QIODevice::WriteOnly));
    brokenReceipt.write("not json");
    brokenReceipt.close();

    const LibraryScan scan = scanLibrary(library.path(), {outside, inLibrary.toUpper(), broken});
    QCOMPARE(scan.receipts.size(), 2);
    QVERIFY(receiptForClient(scan.receipts, QStringLiteral("wow-classic")) != nullptr);
    const Receipt* found = receiptForClient(scan.receipts, QStringLiteral("everquest"));
    QVERIFY(found != nullptr);
    QCOMPARE(found->folder, QDir::toNativeSeparators(QDir(outside).absolutePath()));
    QCOMPARE(scan.problems.size(), 1);
    QVERIFY2(scan.problems.first().contains(QStringLiteral("is not a JSON object")), qPrintable(scan.problems.first()));
  }
};

QTEST_GUILESS_MAIN(InstallLibraryTest)
#include "installLibraryTest.moc"
