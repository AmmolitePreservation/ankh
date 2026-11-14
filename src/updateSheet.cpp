#include "updateSheet.h"

#include "darkTitleBar.h"

#include "installInfo.h"
#include "updateInstaller.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QString notesText(const UpdateOffer& offer)
{
  return offer.notes.isEmpty() ? QStringLiteral("No release notes.") : offer.notes;
}

}

UpdateSheet::UpdateSheet(const UpdateOffer& offer, const QString& currentVersion, QWidget* parent)
  : QDialog(parent),
    installer(new UpdateInstaller(offer, currentVersion, this)),
    heading(new QLabel(QStringLiteral("Update to %1").arg(offer.version), this)),
    notes(new QPlainTextEdit(this)),
    status(new QLabel(this)),
    progress(new QProgressBar(this)),
    updateButton(new QPushButton(QStringLiteral("Update"), this)),
    closeButton(new QPushButton(QStringLiteral("Close"), this)),
    page(offer.page)
{
  setWindowTitle(QStringLiteral("Update Ankh"));
  setMinimumSize(560, 420);

  heading->setProperty("heading", true);
  notes->setReadOnly(true);
  notes->setPlainText(notesText(offer));
  notes->setAccessibleName(QStringLiteral("Release notes"));
  status->setProperty("secondary", true);
  status->setWordWrap(true);
  progress->setVisible(false);
  progress->setTextVisible(false);
  updateButton->setObjectName(QStringLiteral("primary"));
  auto* openPage = new QPushButton(QStringLiteral("Open release page"), this);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addWidget(openPage);
  buttons->addStretch(1);
  buttons->addWidget(closeButton);
  buttons->addWidget(updateButton);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(12);
  layout->addWidget(heading);
  layout->addWidget(notes, 1);
  layout->addWidget(progress);
  layout->addWidget(status);
  layout->addLayout(buttons);

  connect(openPage, &QPushButton::clicked, this, [this] { QDesktopServices::openUrl(page); });
  connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(updateButton, &QPushButton::clicked, this, [this] {
    isWorking = true;
    lastFailure.clear();
    updateButton->setEnabled(false);
    closeButton->setEnabled(false);
    progress->setRange(0, 0);
    progress->setVisible(true);
    installer->start();
  });
  connect(installer, &UpdateInstaller::statusChanged, status, &QLabel::setText);
  connect(installer, &UpdateInstaller::progressChanged, this, [this](qint64 received, qint64 total) {
    progress->setRange(0, total > 0 ? static_cast<int>(total / 1024) : 0);
    progress->setValue(static_cast<int>(received / 1024));
  });
  connect(installer, &UpdateInstaller::installerStarted, this, [this] {
    progress->setRange(0, 0);
  });
  connect(installer, &UpdateInstaller::failed, this, [this](const QString& message) {
    isWorking = false;
    lastFailure = message;
    progress->setVisible(false);
    updateButton->setText(QStringLiteral("Try again"));
    closeButton->setEnabled(true);
    refresh();
  });
  refresh();
}

void UpdateSheet::showEvent(QShowEvent* event)
{
  applyDarkTitleBar(this);
  QDialog::showEvent(event);
}

// The installer owns the status line while it works; otherwise the line says the one thing standing between the
// user and the update, and the button follows it.
void UpdateSheet::refresh()
{
  if (isWorking) {
    return;
  }
  const bool isNotInstalled = installScope() == InstallScope::notInstalled;
  updateButton->setEnabled(!isNotInstalled && heldFor.isEmpty() && !isChecking);
  if (isNotInstalled) {
    status->setText(QStringLiteral("This copy was not installed by the Ankh installer, so it cannot update itself. Install Ankh with the installer first."));
  } else if (!heldFor.isEmpty()) {
    status->setText(QStringLiteral("%1 is still running. Updating Ankh would close it half-way, so wait for it to finish and open this sheet again.")
      .arg(heldFor));
  } else if (isChecking) {
    status->setText(QStringLiteral("Checking for the newest release..."));
  } else if (!lastFailure.isEmpty()) {
    status->setText(lastFailure);
  } else if (!checkProblem.isEmpty()) {
    status->setText(QStringLiteral("Could not confirm this is the newest release: %1").arg(checkProblem));
  } else if (installScope() == InstallScope::allUsers) {
    status->setText(QStringLiteral("Windows will ask for permission to update the all-users install."));
  } else {
    status->setText(QString());
  }
}

void UpdateSheet::holdFor(const QString& activity)
{
  heldFor = activity;
  refresh();
}

void UpdateSheet::setChecking(bool isCheckingNow, const QString& problem)
{
  isChecking = isCheckingNow;
  checkProblem = problem;
  refresh();
}

void UpdateSheet::setOffer(const UpdateOffer& offer)
{
  if (isWorking) {
    return;
  }
  installer->setOffer(offer);
  heading->setText(QStringLiteral("Update to %1").arg(offer.version));
  notes->setPlainText(notesText(offer));
  page = offer.page;
}
