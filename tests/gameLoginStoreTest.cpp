#include "gameLoginStore.h"

#include <QUuid>
#include <QtTest>

class GameLoginStoreTest : public QObject {
  Q_OBJECT

private slots:
  void init()
  {
    game = QStringLiteral("AnkhTest %1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
  }

  void cleanup()
  {
    const GameLoginStore store(game, QStringLiteral("live"));
    for (const QString& name : store.usernames()) {
      store.remove(name);
    }
  }

  void loginsAreListedSortedReadBackReplacedAndRemovedPerEdition()
  {
    const GameLoginStore store(game, QStringLiteral("live"));
    const GameLoginStore otherEdition(game, QStringLiteral("classic"));
    QVERIFY(store.usernames().isEmpty());
    store.save(QStringLiteral("zed"), QStringLiteral("first"));
    store.save(QStringLiteral("Alpha"), QStringLiteral("a/b*c%d"));
    otherEdition.save(QStringLiteral("elsewhere"), QStringLiteral("x"));
    QCOMPARE(store.usernames(), (QStringList{QStringLiteral("Alpha"), QStringLiteral("zed")}));
    QCOMPARE(store.password(QStringLiteral("Alpha")), std::optional<QString>(QStringLiteral("a/b*c%d")));
    QCOMPARE(store.password(QStringLiteral("zed")), std::optional<QString>(QStringLiteral("first")));
    QVERIFY(!store.password(QStringLiteral("nobody")).has_value());
    store.save(QStringLiteral("zed"), QStringLiteral("second"));
    QCOMPARE(store.password(QStringLiteral("zed")), std::optional<QString>(QStringLiteral("second")));
    QCOMPARE(store.usernames().size(), 2);
    store.remove(QStringLiteral("zed"));
    store.remove(QStringLiteral("zed"));
    QCOMPARE(store.usernames(), QStringList{QStringLiteral("Alpha")});
    QCOMPARE(otherEdition.usernames(), QStringList{QStringLiteral("elsewhere")});
    otherEdition.remove(QStringLiteral("elsewhere"));
  }

  void aNameWithSlashesOrStarsStaysItself()
  {
    const GameLoginStore store(game, QStringLiteral("live"));
    store.save(QStringLiteral("odd/na*me"), QStringLiteral("pw"));
    QCOMPARE(store.usernames(), QStringList{QStringLiteral("odd/na*me")});
    QCOMPARE(store.password(QStringLiteral("odd/na*me")), std::optional<QString>(QStringLiteral("pw")));
  }

private:
  QString game;
};

QTEST_GUILESS_MAIN(GameLoginStoreTest)
#include "gameLoginStoreTest.moc"
