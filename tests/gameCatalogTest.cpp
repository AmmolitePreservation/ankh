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
    const World world{3, QStringLiteral("Arcanite"), QStringLiteral("World of Warcraft"), QStringLiteral("classic"),
      QStringLiteral("Classic (1.12.1)"), QColor("#82b556"), 2004, 2006, QStringLiteral("Online, 12 playing"), 2,
      QStringLiteral("Arcanite Logins")};
    const Game game = gameFromWorld(world);
    QCOMPARE(game.id, QStringLiteral("world3"));
    QCOMPARE(game.serverID, 3);
    QCOMPARE(game.title, QStringLiteral("World of Warcraft"));
    QCOMPARE(game.edition, QStringLiteral("Classic (1.12.1)"));
    QCOMPARE(game.variant, QStringLiteral("classic"));
    QCOMPARE(game.worldName, QStringLiteral("Arcanite"));
    QCOMPARE(game.initials, QStringLiteral("WW"));
    QCOMPARE(game.tint, QColor("#82b556"));
    QCOMPARE(game.status, QStringLiteral("Online, 12 playing"));
    QCOMPARE(game.accountDomainID, std::optional<int>(2));
    QVERIFY(!game.isInstalled);
    QCOMPARE(game.executable, QString());
    QVERIFY(!game.givesCredential);
  }

  void aWorldThatSignsThePlayerInCarriesItsProgramAndWhereToAskForTheAccount()
  {
    World world{3, QStringLiteral("Arcanite"), QStringLiteral("World of Warcraft"), QStringLiteral("classic"),
      QStringLiteral("Classic (1.12.1)"), QColor("#82b556"), 2004, 2006, QString(), 2, QStringLiteral("Arcanite Logins")};
    world.executable = QStringLiteral("WoW.exe");
    world.givesCredential = true;
    const Game game = gameFromWorld(world);
    QVERIFY(!game.isAttached);
    QCOMPARE(game.executable, QStringLiteral("WoW.exe"));
    QVERIFY(game.givesCredential);
    QCOMPARE(credentialUrl(QUrl(QStringLiteral("https://ammolite.test:1996")), game),
      QUrl(QStringLiteral("https://ammolite.test:1996/api/v1/servers/3/credential")));
  }

  void theEditionsPackagesAreFoundInTheOrderTheServerGaveThem()
  {
    const Edition classic{QStringLiteral("World of Warcraft"), QStringLiteral("classic"), QStringLiteral("Classic (1.12.1)")};
    const Edition tbc{QStringLiteral("World of Warcraft"), QStringLiteral("tbc"), QStringLiteral("The Burning Crusade (2.4.3)")};
    const Edition pop{QStringLiteral("EverQuest"), QStringLiteral("pop"), QStringLiteral("Planes of Power")};
    const QList<Package> packages = {
      {QStringLiteral("wow-classic"), QStringLiteral("Classic client"), QStringLiteral("client"), {classic}, std::nullopt},
      {QStringLiteral("wow-tbc"), QStringLiteral("Outland client"), QStringLiteral("client"), {tbc}, std::nullopt},
      {QStringLiteral("wow-login"), QStringLiteral("Login"), QStringLiteral("requiredPlugin"), {classic, tbc, pop}, std::nullopt},
      {QStringLiteral("loose"), QStringLiteral("Unattached"), QStringLiteral("optionalPlugin"), {}, std::nullopt},
    };
    Game game = gameFromWorld(World{3, QStringLiteral("Arcanite"), QStringLiteral("World of Warcraft"), QStringLiteral("classic"),
      QStringLiteral("Classic (1.12.1)"), QColor("#82b556"), 2004, 2006, QString(), std::nullopt, QString()});
    QStringList names;
    for (const Package& package : packagesFor(game, packages)) {
      names.append(package.name);
    }
    QCOMPARE(names, QStringList({QStringLiteral("wow-classic"), QStringLiteral("wow-login")}));
    game.variant = QStringLiteral("wotlk");
    QVERIFY(packagesFor(game, packages).isEmpty());
  }

  void anAttachedGameThatSteamStartsCarriesItsAppIDAndStartUrl()
  {
    AttachedGame soma;
    soma.game = QStringLiteral("SOMA");
    soma.variant = QStringLiteral("steam");
    soma.edition = QStringLiteral("Steam");
    soma.executable = QStringLiteral("Soma.exe");
    soma.launcher = QStringLiteral("steam");
    soma.steamAppID = 282140;
    const Game game = gameFromAttached(soma);
    QVERIFY(game.isAttached);
    QVERIFY(game.startsThroughSteam());
    QCOMPARE(game.executable, QStringLiteral("Soma.exe"));
    QCOMPARE(steamStartUrl(game), QUrl(QStringLiteral("steam://rungameid/282140")));

    AttachedGame ddo;
    ddo.game = QStringLiteral("Dungeons & Dragons Online");
    ddo.variant = QStringLiteral("live");
    ddo.launcher = QStringLiteral("turbineGLS");
    ddo.launcherConfig = QStringLiteral("ddo.launcherconfig");
    const Game ownLauncher = gameFromAttached(ddo);
    QVERIFY(ownLauncher.isAttached);
    QVERIFY(!ownLauncher.startsThroughSteam());
    QCOMPARE(ownLauncher.launcherConfig, QStringLiteral("ddo.launcherconfig"));
  }

  void rolesHaveTheirGroupLabels()
  {
    QCOMPARE(roleLabel(QStringLiteral("client")), QStringLiteral("Client"));
    QCOMPARE(roleLabel(QStringLiteral("requiredPlugin")), QStringLiteral("Required plugins"));
    QCOMPARE(roleLabel(QStringLiteral("optionalPlugin")), QStringLiteral("Optional plugins"));
  }
};

QTEST_GUILESS_MAIN(GameCatalogTest)
#include "gameCatalogTest.moc"
