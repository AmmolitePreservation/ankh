#pragma once

#include <QDialog>

class QLabel;
class QLineEdit;
class QPushButton;

class SignInDialog : public QDialog {
  Q_OBJECT

public:
  explicit SignInDialog(QWidget* parent = nullptr);

  void showSignIn(const QString& note);
  void showCreateAccount();
  void showFailure(const QString& message);
  void reset();

signals:
  void signInRequested(const QString& username, const QString& password);
  void createAccountRequested(const QString& inviteCode, const QString& username, const QString& password, const QString& confirmation);

protected:
  void showEvent(QShowEvent* event) override;

private:
  void setCreatingAccount(bool isCreating);
  void submit();
  void setWorking(bool isWorking);

  QLabel* heading;
  QLabel* explanation;
  QLabel* note;
  QLabel* inviteLabel;
  QLabel* confirmationLabel;
  QLineEdit* inviteCode;
  QLineEdit* username;
  QLineEdit* password;
  QLineEdit* confirmation;
  QPushButton* submitButton;
  QPushButton* switchButton;
  QPushButton* cancelButton;
  bool isCreatingAccount = false;
};
