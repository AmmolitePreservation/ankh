#include "userMenuButton.h"

#include <QtTest>

class UserMenuButtonTest : public QObject {
  Q_OBJECT

private slots:
  void theMenuIsNamedForTheUser()
  {
    UserMenuButton button(QStringLiteral("pariator"));
    QCOMPARE(button.accessibleName(), QStringLiteral("pariator menu"));
    button.setUserName(QStringLiteral("Sign in"));
    QCOMPARE(button.accessibleName(), QStringLiteral("Sign in menu"));
  }

  void theButtonGrowsWithTheName()
  {
    UserMenuButton button(QStringLiteral("abc"));
    const int shortWidth = button.width();
    button.setUserName(QStringLiteral("a_much_longer_username_32_chars_"));
    QVERIFY(button.width() > shortWidth + 100);
    QCOMPARE(button.minimumWidth(), button.maximumWidth());
    button.setUserName(QStringLiteral("abc"));
    QCOMPARE(button.width(), shortWidth);
  }
};

QTEST_MAIN(UserMenuButtonTest)
#include "userMenuButtonTest.moc"
