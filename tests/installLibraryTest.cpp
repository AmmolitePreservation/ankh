#include "installLibrary.h"

#include <QDir>
#include <QFile>
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
  receipt.tools.append({QStringLiteral("thorium"), QStringLiteral("2026.11.01-abc1234"), QStringLiteral("5580")});
  receipt.files.insert(QStringLiteral("WoW.exe"), {QStringLiteral("wow-classic"), 5335826980, 0xDEADBEEFu, false});
  receipt.files.insert(QStringLiteral("Data/common.MPQ"), {QStringLiteral("wow-classic"), 12, 7u, false});
  receipt.files.insert(QStringLiteral("WTF/Config.wtf"), {QStringLiteral("wow-classic"), 40, 8u, true});
  receipt.files.insert(QStringLiteral("thorium/thorium.dll"), {QStringLiteral("thorium"), 3, 9u, false});
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
    QCOMPARE(read->tools.size(), 1);
    QCOMPARE(read->tools.first().name, QStringLiteral("thorium"));
    QCOMPARE(read->files.size(), 4);
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).byteSize, qint64(5335826980));
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).crc32, 0xDEADBEEFu);
    QCOMPARE(read->files.value(QStringLiteral("WoW.exe")).isWriteOnce, false);
    QCOMPARE(read->files.value(QStringLiteral("WTF/Config.wtf")).isWriteOnce, true);
    QCOMPARE(read->files.value(QStringLiteral("thorium/thorium.dll")).package, QStringLiteral("thorium"));
  }

  void aReceiptWithoutTheWriteOnceFlagIsRefused()
  {
    QTemporaryDir library;
    const QString folder = QDir(library.path()).filePath(QStringLiteral("Old"));
    QVERIFY(QDir().mkpath(folder + QStringLiteral("/.ankh")));
    QFile old(QDir(folder).filePath(QStringLiteral(".ankh/receipt.json")));
    QVERIFY(old.open(QIODevice::WriteOnly));
    old.write("{\"client\": {\"name\": \"a\", \"version\": \"1\", \"sha256\": \"b\"}, \"tools\": [], "
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
    broken.write("{\"client\": {\"name\": \"everquest\"}, \"tools\": [], \"files\": {}}");
    broken.close();
    QVERIFY(root.mkpath(QStringLiteral("Not an install")));
    QVERIFY(root.mkpath(QStringLiteral(".staging")));
    const LibraryScan scan = scanLibrary(library.path());
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
    const LibraryScan scan = scanLibrary(QStringLiteral("C:/definitely/not/here/ankh"));
    QVERIFY(scan.receipts.isEmpty() && scan.problems.isEmpty());
  }
};

QTEST_GUILESS_MAIN(InstallLibraryTest)
#include "installLibraryTest.moc"
