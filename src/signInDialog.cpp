#include "signInDialog.h"

#include "darkTitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QLabel* makeLabel(const QString& text, const char* property, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setProperty(property, true);
  label->setWordWrap(true);
  return label;
}

QLineEdit* makeField(const QString& name, QLineEdit::EchoMode echo, QWidget* parent)
{
  auto* field = new QLineEdit(parent);
  field->setObjectName(name);
  field->setAccessibleName(name);
  field->setPlaceholderText(name);
  field->setEchoMode(echo);
  return field;
}

}

SignInDialog::SignInDialog(QWidget* parent)
  : QDialog(parent),
    heading(makeLabel(QString(), "heading", this)),
    explanation(makeLabel(QString(), "bodySecondary", this)),
    note(makeLabel(QString(), "secondary", this)),
    inviteLabel(makeLabel(QStringLiteral("An invite code from an existing member."), "secondary", this)),
    confirmationLabel(makeLabel(QStringLiteral("Type the password again."), "secondary", this)),
    inviteCode(makeField(QStringLiteral("Invite code"), QLineEdit::Normal, this)),
    username(makeField(QStringLiteral("Username"), QLineEdit::Normal, this)),
    password(makeField(QStringLiteral("Password"), QLineEdit::Password, this)),
    confirmation(makeField(QStringLiteral("Confirm password"), QLineEdit::Password, this)),
    submitButton(new QPushButton(this)),
    switchButton(new QPushButton(this)),
    cancelButton(new QPushButton(QStringLiteral("Cancel"), this))
{
  setModal(true);
  setWindowTitle(QStringLiteral("Ankh"));
  setFixedWidth(460);
  submitButton->setObjectName(QStringLiteral("primary"));
  submitButton->setDefault(true);
  switchButton->setProperty("link", true);
  switchButton->setProperty("underlined", true);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(10);
  buttons->addStretch(1);
  buttons->addWidget(cancelButton);
  buttons->addWidget(submitButton);
  auto* switchRow = new QHBoxLayout;
  switchRow->addWidget(switchButton);
  switchRow->addStretch(1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(28, 26, 28, 24);
  layout->setSpacing(10);
  layout->addWidget(heading);
  layout->addWidget(explanation);
  layout->addSpacing(6);
  layout->addWidget(inviteCode);
  layout->addWidget(inviteLabel);
  layout->addWidget(username);
  layout->addWidget(password);
  layout->addWidget(confirmation);
  layout->addWidget(confirmationLabel);
  layout->addWidget(note);
  layout->addSpacing(8);
  layout->addLayout(buttons);
  layout->addLayout(switchRow);

  connect(submitButton, &QPushButton::clicked, this, &SignInDialog::submit);
  connect(switchButton, &QPushButton::clicked, this, [this] {
    note->clear();
    setCreatingAccount(!isCreatingAccount);
  });
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  for (QLineEdit* field : {inviteCode, username, password, confirmation}) {
    connect(field, &QLineEdit::returnPressed, this, &SignInDialog::submit);
  }
  setCreatingAccount(false);
}

void SignInDialog::showEvent(QShowEvent* event)
{
  applyDarkTitleBar(this);
  QDialog::showEvent(event);
}

void SignInDialog::setCreatingAccount(bool isCreating)
{
  isCreatingAccount = isCreating;
  heading->setText(isCreating ? QStringLiteral("Create an account") : QStringLiteral("Sign in"));
  explanation->setText(isCreating
    ? QStringLiteral("Accounts are by invitation. You will be signed in once it is created.")
    : QStringLiteral("Sign in with your Ammolite login to see the collection."));
  for (QWidget* widget : {static_cast<QWidget*>(inviteCode), static_cast<QWidget*>(inviteLabel), static_cast<QWidget*>(confirmation), static_cast<QWidget*>(confirmationLabel)}) {
    widget->setVisible(isCreating);
  }
  submitButton->setText(isCreating ? QStringLiteral("Create account") : QStringLiteral("Sign in"));
  switchButton->setText(isCreating ? QStringLiteral("I already have an account") : QStringLiteral("Create an account"));
  (isCreating ? inviteCode : username)->setFocus();
  adjustSize();
}

void SignInDialog::setWorking(bool isWorking)
{
  for (QWidget* widget : {static_cast<QWidget*>(submitButton), static_cast<QWidget*>(switchButton), static_cast<QWidget*>(inviteCode),
         static_cast<QWidget*>(username), static_cast<QWidget*>(password), static_cast<QWidget*>(confirmation)}) {
    widget->setEnabled(!isWorking);
  }
}

void SignInDialog::submit()
{
  if (!submitButton->isEnabled()) {
    return;
  }
  const QString name = username->text().trimmed();
  if (name.isEmpty() || password->text().isEmpty()) {
    note->setText(QStringLiteral("Enter your username and password."));
    return;
  }
  if (!isCreatingAccount) {
    setWorking(true);
    note->setText(QStringLiteral("Signing in..."));
    emit signInRequested(name, password->text());
    return;
  }
  if (inviteCode->text().trimmed().isEmpty()) {
    note->setText(QStringLiteral("Enter the invite code."));
    return;
  }
  if (password->text() != confirmation->text()) {
    note->setText(QStringLiteral("The passwords do not match."));
    return;
  }
  setWorking(true);
  note->setText(QStringLiteral("Creating the account..."));
  emit createAccountRequested(inviteCode->text().trimmed(), name, password->text(), confirmation->text());
}

void SignInDialog::showSignIn(const QString& message)
{
  setWorking(false);
  setCreatingAccount(false);
  note->setText(message);
}

void SignInDialog::showCreateAccount()
{
  setWorking(false);
  setCreatingAccount(true);
  note->clear();
}

void SignInDialog::showFailure(const QString& message)
{
  setWorking(false);
  note->setText(message);
  password->selectAll();
  password->setFocus();
}

void SignInDialog::reset()
{
  setWorking(false);
  inviteCode->clear();
  username->clear();
  password->clear();
  confirmation->clear();
  note->clear();
}
