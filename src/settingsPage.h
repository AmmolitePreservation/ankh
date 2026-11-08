#pragma once

#include <QWidget>

class AccountView;
class QLabel;
class QStackedWidget;

class SettingsPage : public QWidget {
  Q_OBJECT

public:
  SettingsPage(const QString& serviceAddress, QWidget* parent = nullptr);

  AccountView* accountView() const;
  void showSection(int index);
  void setUpdateStatus(const QString& text);

signals:
  void updateCheckRequested();

private:
  QStackedWidget* sections;
  AccountView* account;
  QLabel* updateStatus;
};
