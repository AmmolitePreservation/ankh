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

QString nearestExisting(const QString& folder)
{
  QDir probe(folder);
  while (!probe.exists()) {
    if (!probe.cdUp()) {
      return QString();
    }
  }
  return probe.absolutePath();
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

InstallSheet::InstallSheet(const Game& game, const InstallPlan& plan, QWidget* parent)
  : QDialog(parent),
    rows(new QVBoxLayout),
    folderLabel(new QLabel(this)),
    spaceLabel(new QLabel(this)),
    warning(new QLabel(this)),
    installButton(new QPushButton(plan.isToolChange ? QStringLiteral("Apply") : plan.isUpdate() ? QStringLiteral("Update") : QStringLiteral("Install"), this))
{
  const QString verb = plan.isToolChange ? QStringLiteral("Apply") : plan.isUpdate() ? QStringLiteral("Update") : QStringLiteral("Install");
  setWindowTitle(QStringLiteral("%1 %2").arg(plan.isToolChange ? QStringLiteral("Change tools of") : verb, game.title));
  setMinimumWidth(620);
  auto* heading = makeLabel(plan.isToolChange ? QStringLiteral("Change the tools of %1 %2").arg(game.title, game.edition)
    : QStringLiteral("%1 %2 %3").arg(verb, game.title, game.edition), "heading", this);
  rows->setSpacing(6);
  folderLabel->setWordWrap(true);
  folderLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  spaceLabel->setProperty("secondary", true);
  spaceLabel->setWordWrap(true);
  warning->setProperty("secondary", true);
  warning->setWordWrap(true);
  installButton->setObjectName(QStringLiteral("primary"));
  auto* chooseFolder = new QPushButton(QStringLiteral("Choose another folder..."), this);
  chooseFolder->setToolTip(QStringLiteral("Put this game somewhere else. The library folder in Settings stays as it is."));
  chooseFolder->setVisible(!plan.isUpdate());
  auto* cancelButton = new QPushButton(QStringLiteral("Cancel"), this);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addWidget(chooseFolder);
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
  connect(chooseFolder, &QPushButton::clicked, this, [this] {
    const QString start = nearestExisting(QFileInfo(folderLabel->text()).absolutePath());
    const QString chosen = QFileDialog::getExistingDirectory(this, QStringLiteral("Folder for this game"), start);
    if (!chosen.isEmpty()) {
      emit destinationChosen(QDir::toNativeSeparators(chosen));
    }
  });
  showPlan(plan);
}

void InstallSheet::showPlan(const InstallPlan& plan)
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
    if (plan.isUpdate() && !plan.isToolChange) {
      const QString previous = installedVersion(plan, package);
      versionText = previous.isEmpty() ? QStringLiteral("added, %1").arg(package.latest->version)
        : QStringLiteral("%1 %2 %3").arg(previous, QString(QChar(0x2192)), package.latest->version);
    }
    addRow(package.label + roleNote(package), QStringLiteral("%1, %2 download").arg(versionText, sizeText(package.latest->byteSize)));
  }
  for (const QString& name : plan.removed) {
    addRow(name, plan.isToolChange ? QStringLiteral("will be removed; files kept for your settings stay")
      : QStringLiteral("no longer part of this edition, removed"));
  }
  folderLabel->setText(plan.folder);
  const QString stagingRoot = stagingRootFor(plan);
  const QString existing = nearestExisting(plan.isUpdate() ? plan.folder : stagingRoot);
  const QStorageInfo storage(existing);
  const qint64 needed = bytesStillNeeded(plan, stagingRoot);
  const bool hasRoom = storage.isValid() && storage.bytesAvailable() >= needed;
  spaceLabel->setText(QStringLiteral("%1 to download, %2 once extracted; %3 needed now, %4 free on %5.")
    .arg(sizeText(plan.downloadBytes()), sizeText(plan.extractedBytes()), sizeText(needed),
      storage.isValid() ? sizeText(storage.bytesAvailable()) : QStringLiteral("unknown space"),
      QDir::toNativeSeparators(storage.rootPath())));
  const bool writable = !existing.isEmpty() && QFileInfo(existing).isWritable();
  if (!writable && !plan.isUpdate()) {
    warning->setText(QStringLiteral("%1 cannot be written by this account. Choose another folder for this game, or change the "
      "library folder under Settings, Games.").arg(stagingRoot));
  } else if (!writable) {
    warning->setText(QStringLiteral("%1 cannot be written by this account.").arg(plan.folder));
  } else if (!hasRoom) {
    warning->setText(plan.isUpdate() ? QStringLiteral("Not enough free space on that drive.")
      : QStringLiteral("Not enough free space. Free some, or choose another folder for this game."));
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
