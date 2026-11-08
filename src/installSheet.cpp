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

QString roleNote(const Package& package)
{
  if (package.role == QLatin1String("requiredTool")) {
    return QStringLiteral(" (required tool)");
  }
  if (package.role == QLatin1String("optionalTool")) {
    return QStringLiteral(" (optional tool)");
  }
  return QString();
}

QString installedVersion(const InstallPlan& plan, const Package& package)
{
  if (package.name == plan.existing->client.name) {
    return plan.existing->client.version;
  }
  for (const InstalledPackage& tool : plan.existing->tools) {
    if (tool.name == package.name) {
      return tool.version;
    }
  }
  return QString();
}

}

InstallSheet::InstallSheet(const Game& game, const InstallPlan& plan, const QString& libraryRoot, QWidget* parent)
  : QDialog(parent),
    rows(new QVBoxLayout),
    folderLabel(new QLabel(this)),
    spaceLabel(new QLabel(this)),
    warning(new QLabel(this)),
    installButton(new QPushButton(plan.isUpdate() ? QStringLiteral("Update") : QStringLiteral("Install"), this))
{
  const QString verb = plan.isUpdate() ? QStringLiteral("Update") : QStringLiteral("Install");
  setWindowTitle(QStringLiteral("%1 %2").arg(verb, game.title));
  setMinimumWidth(620);
  auto* heading = makeLabel(QStringLiteral("%1 %2 %3").arg(verb, game.title, game.edition), "heading", this);
  rows->setSpacing(6);
  folderLabel->setWordWrap(true);
  folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  spaceLabel->setProperty("secondary", true);
  spaceLabel->setWordWrap(true);
  warning->setProperty("secondary", true);
  warning->setWordWrap(true);
  installButton->setObjectName(QStringLiteral("primary"));
  auto* changeFolder = new QPushButton(QStringLiteral("Change library folder..."), this);
  changeFolder->setVisible(!plan.isUpdate());
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
  layout->addWidget(makeLabel(plan.isUpdate() ? QStringLiteral("In") : QStringLiteral("Into"), "fieldLabel", this));
  layout->addWidget(folderLabel);
  layout->addWidget(spaceLabel);
  if (plan.isUpdate()) {
    layout->addWidget(makeLabel(QStringLiteral("Files that changed in a package are replaced and files it dropped are removed. "
      "Files a package marks write-once are never overwritten, and anything you added yourself is left alone."), "secondary", this));
  }
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
  auto addRow = [this](const QString& text, const QString& detailText) {
    auto* row = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(makeLabel(text, nullptr, row), 1);
    QLabel* detail = makeLabel(detailText, "bodySecondary", row);
    detail->setWordWrap(false);
    rowLayout->addWidget(detail);
    rows->addWidget(row);
  };
  for (const Package& package : plan.fetch) {
    QString versionText = package.latest->version;
    if (plan.isUpdate()) {
      const QString previous = installedVersion(plan, package);
      versionText = previous.isEmpty() ? QStringLiteral("added, %1").arg(package.latest->version)
        : QStringLiteral("%1 %2 %3").arg(previous, QString(QChar(0x2192)), package.latest->version);
    }
    addRow(package.label + roleNote(package), QStringLiteral("%1, %2 download").arg(versionText, sizeText(package.latest->byteSize)));
  }
  for (const QString& name : plan.removed) {
    addRow(name, QStringLiteral("no longer part of this edition, removed"));
  }
  folderLabel->setText(plan.folder);
  const QString probeFolder = plan.isUpdate() ? plan.folder : libraryRoot;
  const QStorageInfo storage(QDir(probeFolder).exists() ? probeFolder : QFileInfo(probeFolder).absolutePath());
  const qint64 needed = bytesStillNeeded(plan, libraryRoot);
  const bool hasRoom = storage.isValid() && storage.bytesAvailable() >= needed;
  spaceLabel->setText(QStringLiteral("%1 to download, %2 once extracted; %3 needed now, %4 free on %5.")
    .arg(sizeText(plan.downloadBytes()), sizeText(plan.extractedBytes()), sizeText(needed),
      storage.isValid() ? sizeText(storage.bytesAvailable()) : QStringLiteral("unknown space"),
      QDir::toNativeSeparators(storage.rootPath())));
  const bool writable = isWritable(probeFolder);
  if (!writable && !plan.isUpdate()) {
    warning->setText(QStringLiteral("%1 cannot be written by this account (Ankh is installed for all users under Program Files). "
      "Choose a library folder before installing; Ankh keeps the choice.").arg(libraryRoot));
  } else if (!writable) {
    warning->setText(QStringLiteral("%1 cannot be written by this account.").arg(plan.folder));
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
