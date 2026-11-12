#include "pluginsPane.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QPushButton>
#include <QSet>
#include <QStyle>
#include <QVBoxLayout>

namespace {

constexpr int helpWidth = 380;
constexpr int helpMargin = 18;
constexpr int categoryRow = -1;

QLabel* makeLabel(const QString& text, const char* property, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  if (property != nullptr) {
    label->setProperty(property, true);
  }
  label->setWordWrap(true);
  return label;
}

QString sizeText(qint64 bytes)
{
  return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

bool isRequired(const Package& package)
{
  return package.role == QLatin1String("requiredPlugin");
}

QString dot()
{
  return QString(QChar(0x00B7));
}

void setSecondary(QLabel* label, bool isSecondary)
{
  label->setProperty("secondary", isSecondary);
  label->style()->unpolish(label);
  label->style()->polish(label);
}

}

PluginsPane::PluginsPane(QWidget* parent)
  : QWidget(parent),
    intro(makeLabel(QString(), "secondary", this)),
    list(new QListWidget(this)),
    applyButton(new QPushButton(QStringLiteral("Apply changes"), this)),
    resetButton(new QPushButton(QStringLiteral("Reset"), this)),
    helpTitle(makeLabel(QString(), "statusTitle", this)),
    helpRole(makeLabel(QString(), "secondary", this)),
    helpDescription(makeLabel(QString(), nullptr, this)),
    helpScreenshot(new QLabel(this))
{
  list->setObjectName(QStringLiteral("pluginList"));
  list->setSelectionMode(QAbstractItemView::SingleSelection);
  list->setFocusPolicy(Qt::StrongFocus);
  list->setAccessibleName(QStringLiteral("Plugins"));
  applyButton->setObjectName(QStringLiteral("primary"));
  applyButton->setAccessibleName(QStringLiteral("Apply plugin changes"));
  resetButton->setAccessibleName(QStringLiteral("Reset plugin choices"));
  helpDescription->setTextInteractionFlags(Qt::TextSelectableByMouse);
  helpScreenshot->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  helpScreenshot->setWordWrap(true);
  helpScreenshot->setProperty("secondary", true);

  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addStretch(1);
  buttons->addWidget(resetButton);
  buttons->addWidget(applyButton);

  auto* left = new QVBoxLayout;
  left->setSpacing(12);
  left->addWidget(intro);
  left->addWidget(list, 1);
  left->addLayout(buttons);

  auto* help = new QFrame(this);
  help->setObjectName(QStringLiteral("helpBox"));
  help->setFixedWidth(helpWidth);
  auto* helpLayout = new QVBoxLayout(help);
  helpLayout->setContentsMargins(helpMargin, 16, helpMargin, 16);
  helpLayout->setSpacing(10);
  helpLayout->addWidget(helpTitle);
  helpLayout->addWidget(helpRole);
  helpLayout->addWidget(helpDescription);
  helpLayout->addWidget(helpScreenshot);
  helpLayout->addStretch(1);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(30, 14, 30, 16);
  layout->setSpacing(24);
  layout->addLayout(left, 1);
  layout->addWidget(help, 0);

  connect(list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) { showHelp(packageOf(current)); });
  connect(list, &QListWidget::itemChanged, this, [this] {
    if (isRebuilding) {
      return;
    }
    refreshRows();
    refreshButtons();
    emit choiceChanged();
  });
  connect(resetButton, &QPushButton::clicked, this, [this] {
    rebuild(selectedName());
    emit choiceChanged();
  });
  connect(applyButton, &QPushButton::clicked, this, [this] { emit applyRequested(stagedOptionalPlugins()); });
  refreshButtons();
  showHelp(nullptr);
}

const Package* PluginsPane::packageOf(QListWidgetItem* item) const
{
  if (item == nullptr || item->data(Qt::UserRole).toInt() == categoryRow) {
    return nullptr;
  }
  return &plugins.at(item->data(Qt::UserRole).toInt());
}

QString PluginsPane::selectedName() const
{
  const Package* selected = packageOf(list->currentItem());
  return selected != nullptr ? selected->name : QString();
}

bool PluginsPane::isListed(const Package& package) const
{
  return showResearch || isRequired(package) || package.category != researchCategory;
}

void PluginsPane::showPlugins(const QList<Package>& newPlugins, bool newIsInstalled, const QStringList& newInstalledNames,
  const QString& newBusyReason, bool newShowResearch)
{
  QStringList signature;
  for (const Package& package : newPlugins) {
    signature.append(QStringLiteral("%1/%2/%3/%4/%5").arg(package.name, package.latest ? package.latest->version : QString(),
      package.description, package.screenshot ? package.screenshot->sha256 : QString(), package.category));
  }
  signature.append(newShowResearch ? QStringLiteral("research") : QStringLiteral("no research"));
  signature.append(newIsInstalled ? QStringLiteral("installed") : QStringLiteral("not installed"));
  signature.append(newInstalledNames.join(QLatin1Char(',')));
  signature.append(newBusyReason);
  const QString joined = signature.join(QLatin1Char('|'));
  const QString keepSelected = selectedName();
  plugins = newPlugins;
  isInstalled = newIsInstalled;
  installedNames = newInstalledNames;
  busyReason = newBusyReason;
  showResearch = newShowResearch;
  if (joined == shownSignature) {
    return;
  }
  shownSignature = joined;
  rebuild(keepSelected);
}

void PluginsPane::rebuild(const QString& keepSelected)
{
  isRebuilding = true;
  list->clear();
  QListWidgetItem* firstPlugin = nullptr;
  QListWidgetItem* keptPlugin = nullptr;
  for (const QString& category : pluginCategories) {
    bool hasHeading = false;
    for (int index = 0; index < plugins.size(); ++index) {
      const Package& package = plugins.at(index);
      if (package.category != category || !isListed(package)) {
        continue;
      }
      if (!hasHeading) {
        auto* heading = new QListWidgetItem(category, list);
        heading->setData(Qt::UserRole, categoryRow);
        heading->setFlags(Qt::NoItemFlags);
        heading->setSizeHint(QSize(0, 34));
        QFont headingFont = heading->font();
        headingFont.setBold(true);
        heading->setFont(headingFont);
        hasHeading = true;
      }
      auto* item = new QListWidgetItem(list);
      item->setData(Qt::UserRole, index);
      item->setSizeHint(QSize(0, 62));
      Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      if (busyReason.isEmpty() && package.latest.has_value() && !isRequired(package)) {
        flags |= Qt::ItemIsUserCheckable;
      }
      item->setFlags(flags);
      const bool isChecked = isRequired(package) || installedNames.contains(package.name);
      item->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
      if (firstPlugin == nullptr) {
        firstPlugin = item;
      }
      if (package.name == keepSelected) {
        keptPlugin = item;
      }
    }
  }
  refreshRows();
  isRebuilding = false;
  refreshButtons();
  if (firstPlugin == nullptr) {
    showHelp(nullptr);
  } else {
    list->setCurrentItem(keptPlugin != nullptr ? keptPlugin : firstPlugin);
  }
}

void PluginsPane::refreshRows()
{
  const bool wasRebuilding = isRebuilding;
  isRebuilding = true;
  for (int row = 0; row < list->count(); ++row) {
    QListWidgetItem* item = list->item(row);
    if (packageOf(item) == nullptr) {
      continue;
    }
    const Package& package = *packageOf(item);
    const bool isChecked = item->checkState() == Qt::Checked;
    const bool wasInstalled = installedNames.contains(package.name);
    QString status;
    if (!package.latest) {
      status = QStringLiteral("No version published yet");
    } else {
      const QString version = package.latest->version;
      const QString download = sizeText(package.latest->byteSize);
      if (isRequired(package)) {
        status = isInstalled && wasInstalled ? QStringLiteral("%1  %2  installed, required").arg(version, dot())
          : QStringLiteral("%1  %2  installs with the game, required").arg(version, dot());
      } else if (isInstalled && isChecked && !wasInstalled) {
        status = QStringLiteral("%1  %2  will be installed, %3 download").arg(version, dot(), download);
      } else if (isInstalled && !isChecked && wasInstalled) {
        status = QStringLiteral("will be removed");
      } else if (wasInstalled) {
        status = QStringLiteral("%1  %2  installed").arg(version, dot());
      } else if (isChecked) {
        status = QStringLiteral("%1  %2  installs with the game, %3 download").arg(version, dot(), download);
      } else {
        status = QStringLiteral("%1  %2  %3 download").arg(version, dot(), download);
      }
    }
    item->setText(QStringLiteral("%1\n%2").arg(package.label, status));
  }
  isRebuilding = wasRebuilding;
}

QStringList PluginsPane::stagedOptionalPlugins() const
{
  QStringList names;
  for (int row = 0; row < list->count(); ++row) {
    const Package* package = packageOf(list->item(row));
    if (package != nullptr && !isRequired(*package) && list->item(row)->checkState() == Qt::Checked) {
      names.append(package->name);
    }
  }
  for (const Package& package : plugins) {
    if (!isListed(package) && installedNames.contains(package.name)) {
      names.append(package.name);
    }
  }
  return names;
}

QStringList PluginsPane::installedOptionalPlugins() const
{
  QStringList names;
  for (const Package& package : plugins) {
    if (!isRequired(package) && installedNames.contains(package.name)) {
      names.append(package.name);
    }
  }
  return names;
}

QStringList PluginsPane::chosenOptionalPlugins() const
{
  return stagedOptionalPlugins();
}

void PluginsPane::refreshButtons()
{
  const QStringList staged = stagedOptionalPlugins();
  const QStringList installed = installedOptionalPlugins();
  const bool hasChanges = QSet<QString>(staged.cbegin(), staged.cend()) != QSet<QString>(installed.cbegin(), installed.cend());
  applyButton->setVisible(isInstalled);
  resetButton->setVisible(true);
  applyButton->setEnabled(isInstalled && hasChanges && busyReason.isEmpty());
  applyButton->setToolTip(busyReason);
  resetButton->setEnabled(busyReason.isEmpty() && (isInstalled ? hasChanges : !staged.isEmpty()));
  if (plugins.isEmpty()) {
    intro->setText(QStringLiteral("This edition has no plugins."));
  } else if (!busyReason.isEmpty()) {
    intro->setText(busyReason);
  } else if (isInstalled) {
    intro->setText(QStringLiteral("Tick the optional plugins you want and press Apply changes. Unticking an installed plugin removes it; "
      "files it keeps for your settings stay."));
  } else {
    intro->setText(QStringLiteral("Required plugins install with the game. Tick any optional plugins you want installed with it too."));
  }
}

void PluginsPane::showHelp(const Package* package)
{
  if (package == nullptr) {
    helpTitle->setText(QStringLiteral("Plugins"));
    helpRole->clear();
    helpDescription->setText(QStringLiteral("Select a plugin to see what it does."));
    setSecondary(helpDescription, true);
    helpScreenshot->clear();
    helpScreenshot->setVisible(false);
    return;
  }
  helpTitle->setText(package->label);
  QString role = QStringLiteral("%1  %2  %3").arg(
    isRequired(*package) ? QStringLiteral("Required plugin") : QStringLiteral("Optional plugin"), dot(), package->category);
  if (package->latest) {
    role += QStringLiteral("  %1  %2").arg(dot(), package->latest->version);
  }
  helpRole->setText(role);
  helpDescription->setText(package->description.isEmpty() ? QStringLiteral("No description yet.") : package->description);
  setSecondary(helpDescription, package->description.isEmpty());
  if (!package->screenshot) {
    helpScreenshot->clear();
    helpScreenshot->setVisible(false);
    return;
  }
  helpScreenshot->setVisible(true);
  if (screenshots.contains(package->name)) {
    helpScreenshot->setWordWrap(false);
    helpScreenshot->setPixmap(screenshots.value(package->name));
    return;
  }
  helpScreenshot->setPixmap(QPixmap());
  helpScreenshot->setWordWrap(true);
  if (screenshotProblems.contains(package->name)) {
    helpScreenshot->setText(QStringLiteral("The screenshot could not be loaded: %1").arg(screenshotProblems.value(package->name)));
    return;
  }
  helpScreenshot->setText(QStringLiteral("Loading screenshot..."));
  if (!screenshotRequested.value(package->name)) {
    screenshotRequested.insert(package->name, true);
    emit screenshotWanted(*package);
  }
}

void PluginsPane::showScreenshot(const QString& packageName, const QByteArray& image)
{
  QPixmap pixmap;
  if (!pixmap.loadFromData(image)) {
    showScreenshotProblem(packageName, QStringLiteral("the image could not be read"));
    return;
  }
  const qreal ratio = devicePixelRatioF();
  QPixmap scaled = pixmap.scaledToWidth(qRound((helpWidth - 2 * helpMargin) * ratio), Qt::SmoothTransformation);
  scaled.setDevicePixelRatio(ratio);
  screenshots.insert(packageName, scaled);
  const Package* shown = packageOf(list->currentItem());
  if (shown != nullptr && shown->name == packageName) {
    showHelp(shown);
  }
}

void PluginsPane::showScreenshotProblem(const QString& packageName, const QString& message)
{
  screenshotProblems.insert(packageName, message);
  const Package* shown = packageOf(list->currentItem());
  if (shown != nullptr && shown->name == packageName) {
    showHelp(shown);
  }
}
