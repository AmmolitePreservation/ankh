#include "updateSheet.h"

#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

UpdateSheet::UpdateSheet(const QString& version, const QString& notes, const QUrl& page, QWidget* parent)
  : QDialog(parent)
{
  setWindowTitle(QStringLiteral("Update Ankh"));
  setMinimumSize(520, 380);

  auto* heading = new QLabel(QStringLiteral("Update to %1").arg(version), this);
  heading->setProperty("heading", true);
  auto* notesView = new QPlainTextEdit(this);
  notesView->setReadOnly(true);
  notesView->setPlainText(notes.isEmpty() ? QStringLiteral("No release notes.") : notes);
  notesView->setAccessibleName(QStringLiteral("Release notes"));
  auto* reason = new QLabel(QStringLiteral("Installing updates from inside Ankh is not built into this version. Download the installer from the release page."), this);
  reason->setProperty("secondary", true);
  reason->setWordWrap(true);

  auto* openPage = new QPushButton(QStringLiteral("Open release page"), this);
  auto* update = new QPushButton(QStringLiteral("Update"), this);
  update->setObjectName(QStringLiteral("primary"));
  update->setEnabled(false);
  update->setToolTip(reason->text());
  auto* closeButton = new QPushButton(QStringLiteral("Close"), this);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addWidget(openPage);
  buttons->addStretch(1);
  buttons->addWidget(closeButton);
  buttons->addWidget(update);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(12);
  layout->addWidget(heading);
  layout->addWidget(notesView, 1);
  layout->addWidget(reason);
  layout->addLayout(buttons);

  connect(openPage, &QPushButton::clicked, this, [page] { QDesktopServices::openUrl(page); });
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}
