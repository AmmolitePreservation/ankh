#pragma once

#include <QWidget>

class AccountView;
class QLabel;
class QStackedWidget;

class SettingsPage : public QWidget {
  Q_OBJECT

public:
  SettingsPage(const QString& serviceAddress, const QString& libraryRoot, QWidget* parent = nullptr);

  AccountView* accountView() const;
  void showSection(int index);
  void setUpdateStatus(const QString& text);
  void setLibraryRoot(const QString& root);
  void setLibraryProblems(const QStringList& problems);

signals:
  void updateCheckRequested();
  void libraryRootChangeRequested();

private:
  QStackedWidget* sections;
  AccountView* account;
  QLabel* libraryRootLabel;
  QLabel* libraryProblems;
  QLabel* updateStatus;
};
