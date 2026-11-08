#include "gameCatalog.h"

#include <QtTest>

class GameCatalogTest : public QObject {
  Q_OBJECT

private slots:
  void initialsSkipLinkingWordsAndCapAtThree_data()
  {
    QTest::addColumn<QString>("title");
    QTest::addColumn<QString>("initials");
    QTest::newRow("linking word") << QStringLiteral("World of Warcraft") << QStringLiteral("WW");
    QTest::newRow("possessive") << QStringLiteral("Asheron's Call") << QStringLiteral("AC");
    QTest::newRow("three words") << QStringLiteral("Motor City Online") << QStringLiteral("MCO");
    QTest::newRow("four words capped") << QStringLiteral("Star Wars Galaxies Online") << QStringLiteral("SWG");
    QTest::newRow("camel case word") << QStringLiteral("EverQuest") << QStringLiteral("EQ");
    QTest::newRow("plain word") << QStringLiteral("Pyreal") << QStringLiteral("PY");
  }

  void initialsSkipLinkingWordsAndCapAtThree()
  {
    QFETCH(QString, title);
    QFETCH(QString, initials);
    QCOMPARE(initialsFor(title), initials);
  }

  void aWorldBecomesAGameWithItsOwnIdentity()
  {
    const World world{3, QStringLiteral("Arcanite"), QStringLiteral("World of Warcraft"), QStringLiteral("Classic (1.12.1)"),
      QColor("#82b556"), 2004, 2006, QStringLiteral("Online, 12 playing"), 2, QStringLiteral("Arcanite Logins")};
    const Game game = gameFromWorld(world);
    QCOMPARE(game.id, QStringLiteral("world3"));
    QCOMPARE(game.serverID, 3);
    QCOMPARE(game.title, QStringLiteral("World of Warcraft"));
    QCOMPARE(game.edition, QStringLiteral("Classic (1.12.1)"));
    QCOMPARE(game.worldName, QStringLiteral("Arcanite"));
    QCOMPARE(game.initials, QStringLiteral("WW"));
    QCOMPARE(game.tint, QColor("#82b556"));
    QCOMPARE(game.status, QStringLiteral("Online, 12 playing"));
    QCOMPARE(game.accountDomainID, std::optional<int>(2));
    QVERIFY(!game.isInstalled);
  }
};

QTEST_GUILESS_MAIN(GameCatalogTest)
#include "gameCatalogTest.moc"
