#include "credentialStore.h"

#include <QUuid>
#include <QtTest>

class CredentialStoreTest : public QObject {
  Q_OBJECT

private slots:
  void init()
  {
    target = QStringLiteral("AnkhTest/%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
  }

  void cleanup()
  {
    CredentialStore(target).remove();
  }

  void nothingIsStoredAtFirst()
  {
    QVERIFY(!CredentialStore(target).load().has_value());
  }

  void aSavedSecretComesBackExactly()
  {
    const CredentialStore store(target);
    store.save(QStringLiteral("ammolite_Xy-12_abc"));
    QCOMPARE(store.load(), std::optional<QString>(QStringLiteral("ammolite_Xy-12_abc")));
  }

  void savingAgainReplacesTheSecret()
  {
    const CredentialStore store(target);
    store.save(QStringLiteral("first"));
    store.save(QStringLiteral("second"));
    QCOMPARE(store.load(), std::optional<QString>(QStringLiteral("second")));
  }

  void aRemovedSecretIsGoneAndRemovingTwiceIsFine()
  {
    const CredentialStore store(target);
    store.save(QStringLiteral("secret"));
    store.remove();
    QVERIFY(!store.load().has_value());
    store.remove();
  }

  void secretsAreKeptPerTarget()
  {
    const CredentialStore first(target);
    const CredentialStore second(target + QStringLiteral("/other"));
    first.save(QStringLiteral("one"));
    second.save(QStringLiteral("two"));
    QCOMPARE(first.load(), std::optional<QString>(QStringLiteral("one")));
    QCOMPARE(second.load(), std::optional<QString>(QStringLiteral("two")));
    second.remove();
  }

private:
  QString target;
};

QTEST_GUILESS_MAIN(CredentialStoreTest)
#include "credentialStoreTest.moc"
