#include "installSheet.h"

#include "darkTitleBar.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStorageInfo>
#include <QVBoxLayout>

namespace {

QString sizeText(qint64 bytes)
{
  return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

QLabel* makeLabel(const QString& text, const char* role, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  if (role != nullptr) {
    label->setProperty(role, true);
  }
  label->setWordWrap(true);
  return label;
}

// The nearest existing ancestor decides whether the library can be written: the library folder itself is made on
// the first install.
bool isWritable(const QString& folder)
{
  QDir probe(folder);
  while (!probe.exists()) {
    if (!probe.cdUp()) {
      return false;
    }
  }
  return QFileInfo(probe.absolutePath()).isWritable();
}

}

InstallSheet::InstallSheet(const Game& game, const InstallPlan& plan, const QString& libraryRoot, QWidget* parent)
  : QDialog(parent),
    rows(new QVBoxLayout),
    folderLabel(new QLabel(this)),
    spaceLabel(new QLabel(this)),
    warning(new QLabel(this)),
    installButton(new QPushButton(QStringLiteral("Install"), this))
{
  setWindowTitle(QStringLiteral("Install %1").arg(game.title));
  setMinimumWidth(620);
  auto* heading = makeLabel(QStringLiteral("Install %1 %2").arg(game.title, game.edition), "heading", this);
  rows->setSpacing(6);
  folderLabel->setWordWrap(true);
  folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  spaceLabel->setProperty("secondary", true);
  spaceLabel->setWordWrap(true);
  warning->setProperty("secondary", true);
  warning->setWordWrap(true);
  installButton->setObjectName(QStringLiteral("primary"));
  auto* changeFolder = new QPushButton(QStringLiteral("Change library folder..."), this);
  auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), this);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addWidget(changeFolder);
  buttons->addStretch(1);
  buttons->addWidget(cancelButton);
  buttons->addWidget(installButton);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(14);
  layout->addWidget(heading);
  layout->addLayout(rows);
  layout->addWidget(makeLabel(QStringLiteral("Into"), "fieldLabel", this));
  layout->addWidget(folderLabel);
  layout->addWidget(spaceLabel);
  layout->addWidget(warning);
  layout->addStretch(1);
  layout->addLayout(buttons);

  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
  connect(installButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(changeFolder, &QPushButton::clicked, this, [this, libraryRoot] {
    const QString chosen = QFileDialog::getExistingDirectory(this, QStringLiteral("Game library folder"), libraryRoot);
    if (!chosen.isEmpty()) {
      emit libraryRootChosen(QDir::toNativeSeparators(chosen));
    }
  });
  showPlan(plan, libraryRoot);
}

void InstallSheet::showPlan(const InstallPlan& plan, const QString& libraryRoot)
{
  while (QLayoutItem* item = rows->takeAt(0)) {
    delete item->widget();
    delete item;
  }
  for (const Package& package : plan.packages()) {
    auto* row = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(makeLabel(QStringLiteral("%1 %2").arg(package.label, package.role == QLatin1String("client")
      ? QString() : QStringLiteral("(required tool)")).trimmed(), nullptr, row), 1);
    QLabel* detail = makeLabel(QStringLiteral("%1, %2 download").arg(package.latest->version, sizeText(package.latest->byteSize)),
      "bodySecondary", row);
    detail->setWordWrap(false);
    rowLayout->addWidget(detail);
    rows->addWidget(row);
  }
  folderLabel->setText(plan.folder);
  const QStorageInfo storage(QDir(libraryRoot).exists() ? libraryRoot : QFileInfo(libraryRoot).absolutePath());
  const qint64 needed = bytesStillNeeded(plan, libraryRoot);
  const bool hasRoom = storage.isValid() && storage.bytesAvailable() >= needed;
  spaceLabel->setText(QStringLiteral("%1 to download, %2 once installed; %3 needed now, %4 free on %5.")
    .arg(sizeText(plan.downloadBytes()), sizeText(plan.extractedBytes()), sizeText(needed),
      storage.isValid() ? sizeText(storage.bytesAvailable()) : QStringLiteral("unknown space"),
      QDir::toNativeSeparators(storage.rootPath())));
  const bool writable = isWritable(libraryRoot);
  if (!writable) {
    warning->setText(QStringLiteral("%1 cannot be written by this account (Ankh is installed for all users under Program Files). "
      "Choose a library folder before installing; Ankh keeps the choice.").arg(libraryRoot));
  } else if (!hasRoom) {
    warning->setText(QStringLiteral("Not enough free space. Free some, or choose a library folder on another drive."));
  } else {
    warning->setText(QString());
  }
  warning->setVisible(!warning->text().isEmpty());
  installButton->setEnabled(writable && hasRoom);
}

void InstallSheet::showEvent(QShowEvent* event)
{
  applyDarkTitleBar(this);
  QDialog::showEvent(event);
}
