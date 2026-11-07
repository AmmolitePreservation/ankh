#pragma once

#include <QWidget>

class SettingsPane : public QWidget {
  Q_OBJECT

public:
  explicit SettingsPane(QWidget* parent = nullptr);

signals:
  void sectionSelected(int index);
};
