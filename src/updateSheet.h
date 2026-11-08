#pragma once

#include "updateOffer.h"

#include <QDialog>

class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class UpdateInstaller;

class UpdateSheet : public QDialog {
  Q_OBJECT

public:
  UpdateSheet(const UpdateOffer& offer, const QString& currentVersion, QWidget* parent = nullptr);

  void setOffer(const UpdateOffer& offer);

protected:
  void showEvent(QShowEvent* event) override;

private:
  UpdateInstaller* installer;
  QLabel* heading;
  QPlainTextEdit* notes;
  QLabel* status;
  QProgressBar* progress;
  QPushButton* updateButton;
  QPushButton* closeButton;
  QUrl page;
  bool isWorking = false;
};
