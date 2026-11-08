#include "signInDialog.h"

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>
#include <QtTest>

namespace {

QPushButton* buttonLabeled(SignInDialog& dialog, const QString& text)
{
  for (QPushButton* button : dialog.findChildren<QPushButton*>()) {
    if (button->text() == text) {
      return button;
    }
  }
  qFatal("No button labeled %s", qPrintable(text));
  Q_UNREACHABLE();
}

QLineEdit* field(SignInDialog& dialog, const QString& name)
{
  return dialog.findChild<QLineEdit*>(name);
}

bool showsText(SignInDialog& dialog, const QString& text)
{
  for (const QLabel* label : dialog.findChildren<QLabel*>()) {
    if (label->text() == text) {
      return true;
    }
  }
  return false;
}

}

class SignInDialogTest : public QObject {
  Q_OBJECT

private slots:
  void signingInSendsTheTrimmedUsernameAndThePassword()
  {
    SignInDialog dialog;
    QSignalSpy spy(&dialog, &SignInDialog::signInRequested);
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("  druid "));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("pass word"));
    buttonLabeled(dialog, QStringLiteral("Sign in"))->click();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("druid"));
    QCOMPARE(spy.first().at(1).toString(), QStringLiteral("pass word"));
    QVERIFY(!buttonLabeled(dialog, QStringLiteral("Sign in"))->isEnabled());
  }

  void blankFieldsAreRefusedWithoutARequest()
  {
    SignInDialog dialog;
    QSignalSpy spy(&dialog, &SignInDialog::signInRequested);
    buttonLabeled(dialog, QStringLiteral("Sign in"))->click();
    QCOMPARE(spy.count(), 0);
    QVERIFY(showsText(dialog, QStringLiteral("Enter your username and password.")));
    QVERIFY(buttonLabeled(dialog, QStringLiteral("Sign in"))->isEnabled());
  }

  void aFailureReopensTheFormWithTheReason()
  {
    SignInDialog dialog;
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("druid"));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("wrong"));
    buttonLabeled(dialog, QStringLiteral("Sign in"))->click();
    dialog.showFailure(QStringLiteral("Invalid username or password."));
    QVERIFY(showsText(dialog, QStringLiteral("Invalid username or password.")));
    QVERIFY(buttonLabeled(dialog, QStringLiteral("Sign in"))->isEnabled());
    QCOMPARE(field(dialog, QStringLiteral("Username"))->text(), QStringLiteral("druid"));
  }

  void accountCreationFieldsAppearOnlyWhenCreating()
  {
    SignInDialog dialog;
    QVERIFY(!field(dialog, QStringLiteral("Invite code"))->isVisibleTo(&dialog));
    QVERIFY(!field(dialog, QStringLiteral("Confirm password"))->isVisibleTo(&dialog));
    buttonLabeled(dialog, QStringLiteral("Create an account"))->click();
    QVERIFY(field(dialog, QStringLiteral("Invite code"))->isVisibleTo(&dialog));
    QVERIFY(field(dialog, QStringLiteral("Confirm password"))->isVisibleTo(&dialog));
    buttonLabeled(dialog, QStringLiteral("I already have an account"))->click();
    QVERIFY(!field(dialog, QStringLiteral("Invite code"))->isVisibleTo(&dialog));
  }

  void creatingAnAccountSendsTheInviteAndBothPasswords()
  {
    SignInDialog dialog;
    dialog.showCreateAccount();
    QSignalSpy spy(&dialog, &SignInDialog::createAccountRequested);
    field(dialog, QStringLiteral("Invite code"))->setText(QStringLiteral(" ABCD-EFGH-JKLM "));
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("druid"));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("correct-horse-battery"));
    field(dialog, QStringLiteral("Confirm password"))->setText(QStringLiteral("correct-horse-battery"));
    buttonLabeled(dialog, QStringLiteral("Create account"))->click();
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).toString(), QStringLiteral("ABCD-EFGH-JKLM"));
    QCOMPARE(spy.first().at(1).toString(), QStringLiteral("druid"));
    QCOMPARE(spy.first().at(2).toString(), QStringLiteral("correct-horse-battery"));
    QCOMPARE(spy.first().at(3).toString(), QStringLiteral("correct-horse-battery"));
  }

  void mismatchedPasswordsAreRefusedBeforeAnyRequest()
  {
    SignInDialog dialog;
    dialog.showCreateAccount();
    QSignalSpy spy(&dialog, &SignInDialog::createAccountRequested);
    field(dialog, QStringLiteral("Invite code"))->setText(QStringLiteral("ABCD"));
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("druid"));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("one-password"));
    field(dialog, QStringLiteral("Confirm password"))->setText(QStringLiteral("another-password"));
    buttonLabeled(dialog, QStringLiteral("Create account"))->click();
    QCOMPARE(spy.count(), 0);
    QVERIFY(showsText(dialog, QStringLiteral("The passwords do not match.")));
  }

  void aMissingInviteCodeIsRefusedBeforeAnyRequest()
  {
    SignInDialog dialog;
    dialog.showCreateAccount();
    QSignalSpy spy(&dialog, &SignInDialog::createAccountRequested);
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("druid"));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("pw"));
    field(dialog, QStringLiteral("Confirm password"))->setText(QStringLiteral("pw"));
    buttonLabeled(dialog, QStringLiteral("Create account"))->click();
    QCOMPARE(spy.count(), 0);
    QVERIFY(showsText(dialog, QStringLiteral("Enter the invite code.")));
  }

  void showingSignInAgainDropsBackToSigningInWithTheNote()
  {
    SignInDialog dialog;
    dialog.showCreateAccount();
    dialog.showSignIn(QStringLiteral("Your sign-in has ended. Sign in again."));
    QVERIFY(!field(dialog, QStringLiteral("Invite code"))->isVisibleTo(&dialog));
    QVERIFY(showsText(dialog, QStringLiteral("Your sign-in has ended. Sign in again.")));
  }

  void resetForgetsWhatWasTyped()
  {
    SignInDialog dialog;
    field(dialog, QStringLiteral("Username"))->setText(QStringLiteral("druid"));
    field(dialog, QStringLiteral("Password"))->setText(QStringLiteral("secret"));
    dialog.reset();
    QVERIFY(field(dialog, QStringLiteral("Username"))->text().isEmpty());
    QVERIFY(field(dialog, QStringLiteral("Password"))->text().isEmpty());
  }
};

QTEST_MAIN(SignInDialogTest)
#include "signInDialogTest.moc"
