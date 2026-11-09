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
  void holdFor(const QString& activity);
  void setChecking(bool isCheckingNow, const QString& problem);

protected:
  void showEvent(QShowEvent* event) override;

private:
  void refresh();

  UpdateInstaller* installer;
  QLabel* heading;
  QPlainTextEdit* notes;
  QLabel* status;
  QProgressBar* progress;
  QPushButton* updateButton;
  QPushButton* closeButton;
  QUrl page;
  QString heldFor;
  QString checkProblem;
  QString lastFailure;
  bool isChecking = false;
  bool isWorking = false;
};
