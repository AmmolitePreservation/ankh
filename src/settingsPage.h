#pragma once

#include <QWidget>

class QLabel;
class QStackedWidget;

class SettingsPage : public QWidget {
  Q_OBJECT

public:
  explicit SettingsPage(QWidget* parent = nullptr);

  void showSection(int index);
  void setUpdateStatus(const QString& text);

signals:
  void updateCheckRequested();

private:
  QStackedWidget* sections;
  QLabel* updateStatus;
};
