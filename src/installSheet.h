#pragma once

#include "installJob.h"

#include <QDialog>

class QLabel;
class QPushButton;
class QVBoxLayout;

class InstallSheet : public QDialog {
  Q_OBJECT

public:
  InstallSheet(const Game& game, const InstallPlan& plan, QWidget* parent = nullptr);

  void showPlan(const InstallPlan& plan);

signals:
  void destinationChosen(const QString& parentFolder);

protected:
  void showEvent(QShowEvent* event) override;

private:
  QVBoxLayout* rows;
  QLabel* folderLabel;
  QLabel* spaceLabel;
  QLabel* warning;
  QPushButton* installButton;
};
