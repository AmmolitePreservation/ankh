#pragma once

#include <QWidget>

class QButtonGroup;

class SettingsPane : public QWidget {
  Q_OBJECT

public:
  explicit SettingsPane(QWidget* parent = nullptr);

  void selectSection(int index);

signals:
  void sectionSelected(int index);

private:
  QButtonGroup* group;
};
