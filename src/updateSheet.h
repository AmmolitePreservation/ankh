#pragma once

#include "updateChecker.h"

#include <QDialog>

class QLabel;
class QProgressBar;
class QPushButton;
class UpdateInstaller;

class UpdateSheet : public QDialog {
  Q_OBJECT

public:
  UpdateSheet(const UpdateOffer& offer, const QString& currentVersion, QWidget* parent = nullptr);

public slots:
  void reject() override;

private:
  UpdateInstaller* installer;
  QLabel* status;
  QProgressBar* progress;
  QPushButton* updateButton;
  QPushButton* closeButton;
  bool isInstalling = false;
};
