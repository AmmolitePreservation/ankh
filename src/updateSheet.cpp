#include "updateSheet.h"

#include "installInfo.h"
#include "updateInstaller.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

UpdateSheet::UpdateSheet(const UpdateOffer& offer, const QString& currentVersion, QWidget* parent)
  : QDialog(parent),
    installer(new UpdateInstaller(offer, currentVersion, this)),
    status(new QLabel(this)),
    progress(new QProgressBar(this)),
    updateButton(new QPushButton(QStringLiteral("Update"), this)),
    closeButton(new QPushButton(QStringLiteral("Close"), this))
{
  setWindowTitle(QStringLiteral("Update Ankh"));
  setMinimumSize(560, 420);

  auto* heading = new QLabel(QStringLiteral("Update to %1").arg(offer.version), this);
  heading->setProperty("heading", true);
  auto* notesView = new QPlainTextEdit(this);
  notesView->setReadOnly(true);
  notesView->setPlainText(offer.notes.isEmpty() ? QStringLiteral("No release notes.") : offer.notes);
  notesView->setAccessibleName(QStringLiteral("Release notes"));
  status->setProperty("secondary", true);
  status->setWordWrap(true);
  progress->setVisible(false);
  progress->setTextVisible(false);
  if (installScope() == InstallScope::buildFolder) {
    status->setText(QStringLiteral("This copy is running from a build folder, so it cannot update itself. Install Ankh with the installer first."));
    updateButton->setEnabled(false);
  } else if (installScope() == InstallScope::allUsers) {
    status->setText(QStringLiteral("Windows will ask for permission to update the all-users install."));
  }
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
  layout->addWidget(notesView, 1);
  layout->addWidget(progress);
  layout->addWidget(status);
  layout->addLayout(buttons);

  connect(openPage, &QPushButton::clicked, this, [page = offer.page] { QDesktopServices::openUrl(page); });
  connect(closeButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(updateButton, &QPushButton::clicked, this, [this] {
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
    isInstalling = true;
    progress->setRange(0, 0);
  });
  connect(installer, &UpdateInstaller::failed, this, [this](const QString& message) {
    isInstalling = false;
    progress->setVisible(false);
    status->setText(message);
    updateButton->setText(QStringLiteral("Try again"));
    updateButton->setEnabled(true);
    closeButton->setEnabled(true);
  });
}

void UpdateSheet::reject()
{
  if (!isInstalling) {
    QDialog::reject();
  }
}
