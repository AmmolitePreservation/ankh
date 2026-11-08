#include "gamePage.h"

#include "icons.h"

#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

namespace {

constexpr int gutter = 30;

QFrame* makeDivider(QWidget* parent)
{
  auto* line = new QFrame(parent);
  line->setObjectName(QStringLiteral("divider"));
  line->setFixedHeight(1);
  return line;
}

QFrame* makeVerticalDivider(QWidget* parent)
{
  auto* line = new QFrame(parent);
  line->setObjectName(QStringLiteral("divider"));
  line->setFixedSize(1, 30);
  return line;
}

QLabel* makeLabel(const QString& text, const char* property, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  if (property != nullptr) {
    label->setProperty(property, true);
  }
  return label;
}

QLabel* makeFixedLabel(int size, QWidget* parent)
{
  auto* label = new QLabel(parent);
  label->setFixedSize(size, size);
  return label;
}

QWidget* inset(QLayout* layout, QWidget* parent)
{
  auto* widget = new QWidget(parent);
  layout->setContentsMargins(gutter, 0, gutter, 0);
  widget->setLayout(layout);
  return widget;
}

}

GamePage::GamePage(QWidget* parent)
  : QWidget(parent),
    colors(ankhPalette()),
    game(),
    iconLabel(makeFixedLabel(70, this)),
    titleLabel(makeLabel(QString(), "heading", this)),
    subtitleLabel(makeLabel(QString(), "subheading", this)),
    gameOptions(new QPushButton(QStringLiteral("Game options"), this)),
    tabs(new QTabBar(this)),
    pages(new QStackedWidget(this)),
    realmChoice(new QComboBox(this)),
    accountChoice(new QComboBox(this)),
    manageAccounts(new QPushButton(QStringLiteral("Manage accounts"), this)),
    launchSettingsToggle(new QPushButton(QStringLiteral("More launch settings"), this)),
    launchSettingsNote(makeLabel(QStringLiteral("No additional launch settings for this release."), "secondary", this)),
    installedSection(new QWidget(this)),
    downloadsBox(new QWidget(this)),
    downloadsLayout(new QVBoxLayout(downloadsBox)),
    upToDateIcon(makeFixedLabel(30, this)),
    upToDateLabel(makeLabel(QStringLiteral("Up to date"), nullptr, this)),
    compatibilityIcon(makeFixedLabel(30, this)),
    optionalToolsIcon(makeFixedLabel(30, this)),
    checkForUpdates(new QPushButton(QStringLiteral("Check for updates"), this)),
    clientToolsLink(new QPushButton(QStringLiteral("Client tools"), this)),
    statusDot(makeFixedLabel(28, this)),
    statusTitle(makeLabel(QString(), "statusTitle", this)),
    statusDetail(makeLabel(QString(), "secondary", this)),
    progress(new QProgressBar(this)),
    primaryButton(new QPushButton(this))
{
  progress->setTextVisible(false);
  progress->setRange(0, 0);
  progress->setVisible(false);
  progress->setAccessibleName(QStringLiteral("Install progress"));
  statusDetail->setWordWrap(true);
  gameOptions->setLayoutDirection(Qt::RightToLeft);
  gameOptions->setMinimumSize(189, 42);
  auto* optionsMenu = new QMenu(gameOptions);
  optionsMenu->addAction(QStringLiteral("Repair"))->setEnabled(false);
  optionsMenu->addAction(QStringLiteral("Uninstall"))->setEnabled(false);
  gameOptions->setMenu(optionsMenu);

  for (const QString& name : {QStringLiteral("Play"), QStringLiteral("Characters"), QStringLiteral("Client tools"), QStringLiteral("Media"), QStringLiteral("About")}) {
    tabs->addTab(name);
  }
  tabs->setExpanding(false);
  tabs->setDrawBase(false);

  manageAccounts->setProperty("link", true);
  manageAccounts->setProperty("underlined", true);
  manageAccounts->setEnabled(false);
  manageAccounts->setToolTip(QStringLiteral("Game accounts are managed on the Ammolite site for now."));
  launchSettingsToggle->setProperty("disclosure", true);
  launchSettingsToggle->setCheckable(true);
  launchSettingsToggle->setIconSize(QSize(18, 18));
  launchSettingsNote->setVisible(false);
  downloadsLayout->setContentsMargins(0, 0, 0, 0);
  downloadsLayout->setSpacing(8);
  checkForUpdates->setLayoutDirection(Qt::RightToLeft);
  checkForUpdates->setMinimumHeight(42);
  checkForUpdates->setToolTip(QStringLiteral("Ask Ammolite for the newest packages now."));
  upToDateLabel->setWordWrap(false);
  upToDateLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  clientToolsLink->setProperty("link", true);
  clientToolsLink->setLayoutDirection(Qt::RightToLeft);
  realmChoice->setAccessibleName(QStringLiteral("Realm"));
  accountChoice->setAccessibleName(QStringLiteral("Account"));
  realmChoice->setMinimumHeight(42);
  accountChoice->setMinimumHeight(42);
  primaryButton->setObjectName(QStringLiteral("primary"));
  primaryButton->setProperty("large", true);
  primaryButton->setMinimumSize(206, 60);
  primaryButton->setIconSize(QSize(22, 22));

  auto* identity = new QVBoxLayout;
  identity->setSpacing(2);
  identity->addStretch(1);
  identity->addWidget(titleLabel);
  identity->addWidget(subtitleLabel);
  identity->addStretch(1);
  auto* header = new QHBoxLayout;
  header->setSpacing(16);
  header->addWidget(iconLabel);
  header->addLayout(identity, 1);
  header->addWidget(gameOptions, 0, Qt::AlignTop);
  header->setContentsMargins(gutter, 0, gutter, 0);

  auto* launchSetup = new QWidget(this);
  auto* launchLayout = new QGridLayout(launchSetup);
  launchLayout->setContentsMargins(0, 0, 0, 0);
  launchLayout->setHorizontalSpacing(52);
  launchLayout->setVerticalSpacing(8);
  launchLayout->addWidget(makeLabel(QStringLiteral("Realm"), "fieldLabel", launchSetup), 0, 0);
  launchLayout->addWidget(makeLabel(QStringLiteral("Account"), "fieldLabel", launchSetup), 0, 1);
  launchLayout->addWidget(realmChoice, 1, 0);
  launchLayout->addWidget(accountChoice, 1, 1);
  launchLayout->addWidget(manageAccounts, 2, 1, Qt::AlignLeft);
  launchLayout->setColumnStretch(0, 1);
  launchLayout->setColumnStretch(1, 1);

  auto* statusRow = new QHBoxLayout;
  statusRow->setSpacing(12);
  statusRow->addWidget(upToDateIcon);
  statusRow->addWidget(upToDateLabel, 1);
  statusRow->addSpacing(20);
  statusRow->addWidget(makeVerticalDivider(installedSection));
  statusRow->addSpacing(20);
  statusRow->addWidget(compatibilityIcon);
  statusRow->addWidget(makeLabel(QStringLiteral("Required compatibility ready"), nullptr, installedSection));
  statusRow->addSpacing(20);
  statusRow->addWidget(checkForUpdates);

  auto* toolsRow = new QHBoxLayout;
  toolsRow->setSpacing(12);
  toolsRow->addWidget(optionalToolsIcon);
  toolsRow->addWidget(makeLabel(QStringLiteral("Optional tools"), nullptr, installedSection));
  toolsRow->addWidget(makeLabel(QStringLiteral("%1   None enabled").arg(QChar(0x00B7)), "bodySecondary", installedSection));
  toolsRow->addStretch(1);
  toolsRow->addWidget(clientToolsLink);

  auto* installedLayout = new QVBoxLayout(installedSection);
  installedLayout->setContentsMargins(0, 0, 0, 0);
  installedLayout->setSpacing(16);
  installedLayout->addWidget(makeLabel(QStringLiteral("Installed client"), "sectionHeading", installedSection));
  installedLayout->addLayout(statusRow);
  installedLayout->addWidget(makeDivider(installedSection));
  installedLayout->addLayout(toolsRow);
  installedLayout->addWidget(makeDivider(installedSection));

  auto* playContent = new QWidget(this);
  auto* playLayout = new QVBoxLayout(playContent);
  playLayout->setContentsMargins(gutter, 14, gutter, 16);
  playLayout->setSpacing(20);
  playLayout->addWidget(makeLabel(QStringLiteral("Launch setup"), "sectionHeading", playContent));
  playLayout->addWidget(launchSetup);
  playLayout->addWidget(makeDivider(playContent));
  playLayout->addWidget(makeLabel(QStringLiteral("Downloads"), "sectionHeading", playContent));
  playLayout->addWidget(downloadsBox);
  playLayout->addWidget(makeDivider(playContent));
  playLayout->addWidget(launchSettingsToggle, 0, Qt::AlignLeft);
  playLayout->addWidget(launchSettingsNote);
  playLayout->addWidget(makeDivider(playContent));
  playLayout->addWidget(installedSection);
  playLayout->addStretch(1);

  auto* playScroll = new QScrollArea(this);
  playScroll->setWidgetResizable(true);
  playScroll->setWidget(playContent);
  playScroll->setFrameShape(QFrame::NoFrame);

  pages->addWidget(playScroll);
  for (int index = 1; index < tabs->count(); ++index) {
    auto* unavailable = makeLabel(QStringLiteral("Not available in this build."), "secondary", this);
    unavailable->setAlignment(Qt::AlignCenter);
    pages->addWidget(unavailable);
  }

  auto* tabRow = new QHBoxLayout;
  tabRow->addWidget(tabs);
  tabRow->addStretch(1);
  tabRow->setContentsMargins(gutter - 12, 0, gutter, 0);
  auto* tabLine = new QHBoxLayout;
  tabLine->addWidget(makeDivider(this));
  tabLine->setContentsMargins(gutter, 0, gutter, 0);

  auto* body = new QWidget(this);
  auto* bodyLayout = new QVBoxLayout(body);
  bodyLayout->setContentsMargins(0, 28, 0, 0);
  bodyLayout->setSpacing(0);
  bodyLayout->addLayout(header);
  bodyLayout->addSpacing(14);
  bodyLayout->addLayout(tabRow);
  bodyLayout->addLayout(tabLine);
  bodyLayout->addWidget(pages, 1);

  auto* statusText = new QVBoxLayout;
  statusText->setSpacing(2);
  statusText->addStretch(1);
  statusText->addWidget(statusTitle);
  statusText->addWidget(statusDetail);
  statusText->addWidget(progress);
  statusText->addStretch(1);
  auto* bottomRow = new QHBoxLayout;
  bottomRow->setSpacing(14);
  bottomRow->addWidget(statusDot);
  bottomRow->addLayout(statusText, 1);
  bottomRow->addWidget(primaryButton);

  auto* bottomBar = new QWidget(this);
  bottomBar->setObjectName(QStringLiteral("bottomBar"));
  bottomBar->setFixedHeight(122);
  auto* bottomLayout = new QVBoxLayout(bottomBar);
  bottomLayout->setContentsMargins(0, 0, 0, 0);
  bottomLayout->setSpacing(0);
  auto* bottomLine = new QHBoxLayout;
  bottomLine->addWidget(makeDivider(bottomBar));
  bottomLine->setContentsMargins(gutter, 0, gutter, 0);
  bottomLayout->addLayout(bottomLine);
  bottomRow->setContentsMargins(gutter, 0, gutter, 0);
  bottomLayout->addLayout(bottomRow, 1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(body, 1);
  layout->addWidget(bottomBar);

  connect(tabs, &QTabBar::currentChanged, pages, &QStackedWidget::setCurrentIndex);
  connect(clientToolsLink, &QPushButton::clicked, this, [this] { tabs->setCurrentIndex(2); });
  connect(checkForUpdates, &QPushButton::clicked, this, &GamePage::updateCheckRequested);
  connect(launchSettingsToggle, &QPushButton::toggled, this, [this](bool isShown) {
    launchSettingsNote->setVisible(isShown);
    refreshIcons();
  });
  connect(accountChoice, &QComboBox::currentIndexChanged, this, [this] { updateStatus(); });
  connect(primaryButton, &QPushButton::clicked, this, [this] {
    switch (installState) {
    case InstallState::installed:
      statusDetail->setText(QStringLiteral("Launching is not built yet."));
      break;
    case InstallState::installing:
      emit cancelRequested();
      break;
    case InstallState::notInstalled:
      emit installRequested(game);
      break;
    case InstallState::updateAvailable:
      emit updateRequested(game);
      break;
    case InstallState::unavailable:
    case InstallState::busyElsewhere:
      break;
    }
  });
  refreshIcons();
}

void GamePage::applyPalette(const Palette& newColors)
{
  colors = newColors;
  refreshIcons();
}

void GamePage::refreshIcons()
{
  const qreal ratio = devicePixelRatioF();
  upToDateIcon->setPixmap(iconPixmap(IconKind::checkBadge, 30, QColor(), ratio));
  compatibilityIcon->setPixmap(iconPixmap(IconKind::checkBadge, 30, QColor(), ratio));
  optionalToolsIcon->setPixmap(iconPixmap(IconKind::settings, 28, colors.text, ratio));
  statusDot->setPixmap(iconPixmap(IconKind::statusDot, 28, game.status.startsWith(QStringLiteral("Online")) ? colors.success : colors.secondary, ratio));
  gameOptions->setIcon(makeIcon(IconKind::chevronDown, 18, colors.text));
  checkForUpdates->setIcon(makeIcon(IconKind::chevronDown, 18, colors.text));
  clientToolsLink->setIcon(makeIcon(IconKind::chevronRight, 18, colors.accent));
  launchSettingsToggle->setIcon(makeIcon(launchSettingsToggle->isChecked() ? IconKind::chevronDown : IconKind::chevronRight, 18, colors.text));
  primaryButton->setIcon(installState == InstallState::installed ? makeIcon(IconKind::play, 22, QColor("#16120A")) : QIcon());

  if (!game.id.isEmpty()) {
    QPixmap icon(QSize(70, 70) * ratio);
    icon.setDevicePixelRatio(ratio);
    icon.fill(Qt::transparent);
    QPainter iconPainter(&icon);
    paintGameIcon(iconPainter, QRect(0, 0, 70, 70), game.initials, game.tint);
    iconPainter.end();
    iconLabel->setPixmap(icon);
  }
}

void GamePage::updateStatus()
{
  statusTitle->setText(game.status.isEmpty() ? QStringLiteral("Status not reported") : game.status);
  const bool isInstalled = installState == InstallState::installed || installState == InstallState::updateAvailable
    || (installState == InstallState::installing && game.isInstalled);
  installedSection->setVisible(isInstalled);
  progress->setVisible(installState == InstallState::installing);
  primaryButton->setEnabled(true);
  primaryButton->setToolTip(QString());
  upToDateLabel->setText(QStringLiteral("Up to date"));
  upToDateLabel->setToolTip(QString());
  switch (installState) {
  case InstallState::installed:
    if (!installDetail.isEmpty()) {
      upToDateLabel->setText(installDetail);
      upToDateLabel->setToolTip(installDetail);
    }
    statusDetail->setText(QStringLiteral("%1 %2 %3").arg(realmChoice->currentText(), QString(QChar(0x00B7)), accountChoice->currentText()));
    primaryButton->setText(QStringLiteral("  Play"));
    break;
  case InstallState::updateAvailable:
    upToDateLabel->setText(QStringLiteral("Update available"));
    statusDetail->setText(installDetail);
    primaryButton->setText(QStringLiteral("Update"));
    break;
  case InstallState::installing:
    statusDetail->setText(installDetail);
    primaryButton->setText(QStringLiteral("Cancel"));
    break;
  case InstallState::notInstalled:
    statusDetail->setText(installDetail.isEmpty() ? QStringLiteral("Not installed.") : installDetail);
    primaryButton->setText(QStringLiteral("Install"));
    break;
  case InstallState::unavailable:
    statusDetail->setText(installDetail);
    primaryButton->setText(QStringLiteral("Install"));
    primaryButton->setEnabled(false);
    primaryButton->setToolTip(installDetail);
    break;
  case InstallState::busyElsewhere:
    statusDetail->setText(installDetail);
    primaryButton->setText(QStringLiteral("Install"));
    primaryButton->setEnabled(false);
    break;
  }
  accountChoice->setEnabled((installState == InstallState::installed || installState == InstallState::updateAvailable)
    && accountsProblem.isEmpty() && game.accountDomainID.has_value()
    && accountChoice->count() > 0 && accountChoice->itemText(0) != QStringLiteral("No game account yet"));
  refreshIcons();
}

void GamePage::setInstallState(InstallState state, const QString& detail)
{
  installState = state;
  installDetail = detail;
  updateStatus();
}

void GamePage::showInstallProgress(const QString& text, qint64 done, qint64 total)
{
  installDetail = text;
  statusDetail->setText(total > 0 ? QStringLiteral("%1 %2%").arg(text).arg(done * 100 / total) : text);
  progress->setRange(0, total > 0 ? 10000 : 0);
  progress->setValue(total > 0 ? static_cast<int>(done * 10000 / total) : 0);
}

void GamePage::refreshAccounts()
{
  accountChoice->clear();
  if (!accountsProblem.isEmpty()) {
    accountChoice->addItem(QStringLiteral("Could not load game accounts"));
    accountChoice->setToolTip(accountsProblem);
    accountChoice->setEnabled(false);
    return;
  }
  accountChoice->setToolTip(QString());
  if (!game.accountDomainID) {
    accountChoice->addItem(QStringLiteral("This realm has no shared logins"));
    accountChoice->setEnabled(false);
    return;
  }
  for (const AccountDomain& domain : accountDomains) {
    if (domain.accountDomainID != *game.accountDomainID) {
      continue;
    }
    for (const GameLogin& login : domain.logins) {
      accountChoice->addItem(login.gameUsername);
    }
  }
  if (accountChoice->count() == 0) {
    accountChoice->addItem(QStringLiteral("No game account yet"));
    accountChoice->setEnabled(false);
  }
}

void GamePage::clearDownloads()
{
  while (QLayoutItem* item = downloadsLayout->takeAt(0)) {
    delete item->widget();
    delete item;
  }
}

void GamePage::showDownloads()
{
  clearDownloads();
  if (game.id.isEmpty()) {
    return;
  }
  const QList<Package> attached = packagesFor(game, packages);
  if (attached.isEmpty()) {
    downloadsLayout->addWidget(makeLabel(QStringLiteral("Nothing is attached to this edition yet."), "secondary", downloadsBox));
    return;
  }
  QString shownRole;
  for (const Package& package : attached) {
    if (package.role != shownRole) {
      shownRole = package.role;
      downloadsLayout->addWidget(makeLabel(roleLabel(package.role), "fieldLabel", downloadsBox));
    }
    auto* row = new QWidget(downloadsBox);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->addWidget(makeLabel(package.label, nullptr, row), 1);
    const QString detail = package.latest
      ? QStringLiteral("%1 %2 %3").arg(package.latest->version, QString(QChar(0x00B7)),
          QLocale().formattedDataSize(package.latest->byteSize, 1, QLocale::DataSizeTraditionalFormat))
      : QStringLiteral("No version published yet");
    rowLayout->addWidget(makeLabel(detail, "bodySecondary", row));
    downloadsLayout->addWidget(row);
  }
}

void GamePage::setPackages(const QList<Package>& newPackages)
{
  packages = newPackages;
  showDownloads();
}

void GamePage::refreshGame(const Game& newGame)
{
  game = newGame;
  updateStatus();
}

void GamePage::setGameAccounts(const QList<AccountDomain>& domains)
{
  accountDomains = domains;
  accountsProblem.clear();
  refreshAccounts();
  updateStatus();
}

void GamePage::setGameAccountsFailed(const QString& message)
{
  accountsProblem = message;
  refreshAccounts();
}

void GamePage::showGame(const Game& newGame)
{
  game = newGame;
  titleLabel->setText(game.title);
  subtitleLabel->setText(game.edition);
  realmChoice->clear();
  realmChoice->addItem(game.worldName);
  realmChoice->setEnabled(false);
  refreshAccounts();
  showDownloads();
  tabs->setCurrentIndex(0);
  updateStatus();
}
