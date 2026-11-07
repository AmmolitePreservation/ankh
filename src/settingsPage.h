#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QStackedWidget;

class SettingsPage : public QWidget {
  Q_OBJECT

public:
  SettingsPage(const QString& currentSkin, QWidget* parent = nullptr);

  void showSection(int index);
  void setUpdateStatus(const QString& text);

signals:
  void skinChosen(const QString& name);
  void updateCheckRequested();

private:
  QStackedWidget* sections;
  QLabel* updateStatus;
};
