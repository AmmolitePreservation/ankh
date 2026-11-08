#pragma once

#include "ammoliteClient.h"

#include <QWidget>

class QLabel;
class QStackedWidget;

class AccountView : public QWidget {
  Q_OBJECT

public:
  AccountView(const QString& serviceAddress, QWidget* parent = nullptr);

  void showSignedOut(const QString& note);
  void showSignedIn(const SignedInUser& user);

signals:
  void signInRequested();
  void createAccountRequested();
  void signOutRequested();

private:
  QStackedWidget* states;
  QLabel* note;
  QLabel* signedInName;
  QLabel* signedInRole;
};
