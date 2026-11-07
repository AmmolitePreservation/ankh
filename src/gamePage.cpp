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
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

namespace {

QFrame* makeDivider(QWidget* parent)
{
  auto* line = new QFrame(parent);
  line->setObjectName(QStringLiteral("divider"));
  line->setFixedHeight(1);
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

}

GamePage::GamePage(QWidget* parent)
  : QWidget(parent),
    colors(ankhPalette()),
    game(),
    iconLabel(makeFixedLabel(64, this)),
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
    characterSection(new QWidget(this)),
    installNote(makeLabel(QStringLiteral("Install this game to choose a realm and an account."), "secondary", this)),
    upToDateIcon(makeFixedLabel(26, this)),
    compatibilityIcon(makeFixedLabel(26, this)),
    optionalToolsIcon(makeFixedLabel(26, this)),
    portraitLabel(makeFixedLabel(60, this)),
    characterName(makeLabel(QString(), "characterName", this)),
    characterSummary(makeLabel(QString(), "secondary", this)),
    checkForUpdates(new QPushButton(QStringLiteral("Check for updates"), this)),
    clientToolsLink(new QPushButton(QStringLiteral("Client tools"), this)),
    statusDot(makeFixedLabel(20, this)),
    statusTitle(makeLabel(QString(), "statusTitle", this)),
    statusDetail(makeLabel(QString(), "secondary", this)),
    primaryButton(new QPushButton(this))
{
  gameOptions->setLayoutDirection(Qt::RightToLeft);
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
  manageAccounts->setEnabled(false);
  manageAccounts->setToolTip(QStringLiteral("Account management is not built yet."));
  launchSettingsToggle->setProperty("disclosure", true);
  launchSettingsToggle->setCheckable(true);
  launchSettingsToggle->setCursor(Qt::PointingHandCursor);
  launchSettingsNote->setVisible(false);
  installNote->setVisible(false);
  checkForUpdates->setLayoutDirection(Qt::RightToLeft);
  checkForUpdates->setEnabled(false);
  checkForUpdates->setToolTip(QStringLiteral("Update checks for games are not built yet."));
  clientToolsLink->setProperty("link", true);
  clientToolsLink->setLayoutDirection(Qt::RightToLeft);
  clientToolsLink->setCursor(Qt::PointingHandCursor);
  realmChoice->setAccessibleName(QStringLiteral("Realm"));
  accountChoice->setAccessibleName(QStringLiteral("Account"));
  realmChoice->setMinimumHeight(44);
  accountChoice->setMinimumHeight(44);
  primaryButton->setObjectName(QStringLiteral("primary"));
  primaryButton->setProperty("large", true);
  primaryButton->setMinimumSize(206, 60);

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

  auto* launchSetup = new QWidget(this);
  auto* launchLayout = new QGridLayout(launchSetup);
  launchLayout->setContentsMargins(0, 0, 0, 0);
  launchLayout->setHorizontalSpacing(48);
  launchLayout->setVerticalSpacing(8);
  launchLayout->addWidget(makeLabel(QStringLiteral("Realm"), "secondary", launchSetup), 0, 0);
  launchLayout->addWidget(makeLabel(QStringLiteral("Account"), "secondary", launchSetup), 0, 1);
  launchLayout->addWidget(realmChoice, 1, 0);
  launchLayout->addWidget(accountChoice, 1, 1);
  launchLayout->addWidget(manageAccounts, 2, 1, Qt::AlignLeft);
  launchLayout->setColumnStretch(0, 1);
  launchLayout->setColumnStretch(1, 1);

  auto* statusRow = new QHBoxLayout;
  statusRow->setSpacing(10);
  statusRow->addWidget(upToDateIcon);
  statusRow->addWidget(makeLabel(QStringLiteral("Up to date"), nullptr, installedSection));
  statusRow->addSpacing(24);
  statusRow->addWidget(compatibilityIcon);
  statusRow->addWidget(makeLabel(QStringLiteral("Required compatibility ready"), nullptr, installedSection));
  statusRow->addStretch(1);
  statusRow->addWidget(checkForUpdates);

  auto* toolsRow = new QHBoxLayout;
  toolsRow->setSpacing(12);
  toolsRow->addWidget(optionalToolsIcon);
  toolsRow->addWidget(makeLabel(QStringLiteral("Optional tools"), nullptr, installedSection));
  toolsRow->addWidget(makeLabel(QStringLiteral("·   None enabled"), "secondary", installedSection));
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

  auto* characterText = new QVBoxLayout;
  characterText->setSpacing(2);
  characterText->addStretch(1);
  characterText->addWidget(characterName);
  characterText->addWidget(characterSummary);
  characterText->addStretch(1);
  auto* characterRow = new QHBoxLayout;
  characterRow->setSpacing(16);
  characterRow->addWidget(portraitLabel);
  characterRow->addLayout(characterText, 1);
  auto* characterLayout = new QVBoxLayout(characterSection);
  characterLayout->setContentsMargins(0, 0, 0, 0);
  characterLayout->setSpacing(16);
  characterLayout->addWidget(makeLabel(QStringLiteral("Recent character"), "sectionHeading", characterSection));
  characterLayout->addLayout(characterRow);

  auto* playContent = new QWidget(this);
  auto* playLayout = new QVBoxLayout(playContent);
  playLayout->setContentsMargins(0, 8, 12, 16);
  playLayout->setSpacing(20);
  playLayout->addWidget(makeLabel(QStringLiteral("Launch setup"), "sectionHeading", playContent));
  playLayout->addWidget(launchSetup);
  playLayout->addWidget(installNote);
  playLayout->addWidget(makeDivider(playContent));
  playLayout->addWidget(launchSettingsToggle, 0, Qt::AlignLeft);
  playLayout->addWidget(launchSettingsNote);
  playLayout->addWidget(makeDivider(playContent));
  playLayout->addWidget(installedSection);
  playLayout->addWidget(characterSection);
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

  auto* body = new QWidget(this);
  auto* bodyLayout = new QVBoxLayout(body);
  bodyLayout->setContentsMargins(28, 24, 28, 0);
  bodyLayout->setSpacing(12);
  bodyLayout->addLayout(header);
  bodyLayout->addSpacing(8);
  bodyLayout->addWidget(tabs);
  bodyLayout->addWidget(makeDivider(body));
  bodyLayout->addWidget(pages, 1);

  auto* statusText = new QVBoxLayout;
  statusText->setSpacing(2);
  statusText->addStretch(1);
  statusText->addWidget(statusTitle);
  statusText->addWidget(statusDetail);
  statusText->addStretch(1);
  auto* bottomBar = new QWidget(this);
  bottomBar->setObjectName(QStringLiteral("bottomBar"));
  bottomBar->setFixedHeight(104);
  auto* bottomLayout = new QHBoxLayout(bottomBar);
  bottomLayout->setContentsMargins(28, 0, 28, 0);
  bottomLayout->setSpacing(14);
  bottomLayout->addWidget(statusDot);
  bottomLayout->addLayout(statusText, 1);
  bottomLayout->addWidget(primaryButton);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(body, 1);
  layout->addWidget(bottomBar);

  connect(tabs, &QTabBar::currentChanged, pages, &QStackedWidget::setCurrentIndex);
  connect(clientToolsLink, &QPushButton::clicked, this, [this] { tabs->setCurrentIndex(2); });
  connect(launchSettingsToggle, &QPushButton::toggled, this, [this](bool isShown) {
    launchSettingsNote->setVisible(isShown);
    refreshIcons();
  });
  connect(realmChoice, &QComboBox::currentIndexChanged, this, [this] { updateStatus(); });
  connect(accountChoice, &QComboBox::currentIndexChanged, this, [this] { updateStatus(); });
  connect(primaryButton, &QPushButton::clicked, this, [this] {
    if (game.isInstalled) {
      statusDetail->setText(QStringLiteral("Launching is not built yet."));
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
  const QColor green("#3FA66B");
  upToDateIcon->setPixmap(iconPixmap(IconKind::check, 26, green, ratio));
  compatibilityIcon->setPixmap(iconPixmap(IconKind::check, 26, green, ratio));
  optionalToolsIcon->setPixmap(iconPixmap(IconKind::settings, 26, colors.secondary, ratio));
  statusDot->setPixmap(iconPixmap(IconKind::dot, 20, game.isInstalled ? green : colors.secondary, ratio));
  gameOptions->setIcon(makeIcon(IconKind::chevronDown, 18, colors.secondary));
  checkForUpdates->setIcon(makeIcon(IconKind::chevronDown, 18, colors.secondary));
  clientToolsLink->setIcon(makeIcon(IconKind::chevronRight, 18, colors.accent));
  launchSettingsToggle->setIcon(makeIcon(launchSettingsToggle->isChecked() ? IconKind::chevronDown : IconKind::chevronRight, 18, colors.secondary));
  primaryButton->setIcon(game.isInstalled ? makeIcon(IconKind::play, 22, colors.base) : QIcon());
  primaryButton->setIconSize(QSize(22, 22));

  QPixmap portrait(QSize(60, 60) * ratio);
  portrait.setDevicePixelRatio(ratio);
  portrait.fill(Qt::transparent);
  QPainter painter(&portrait);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(Qt::NoPen);
  painter.setBrush(colors.raised);
  painter.drawEllipse(QRectF(0.5, 0.5, 59, 59));
  painter.drawPixmap(QRect(14, 14, 32, 32), iconPixmap(IconKind::user, 32, colors.secondary, ratio));
  painter.end();
  portraitLabel->setPixmap(portrait);

  if (!game.id.isEmpty()) {
    QPixmap icon(QSize(64, 64) * ratio);
    icon.setDevicePixelRatio(ratio);
    icon.fill(Qt::transparent);
    QPainter iconPainter(&icon);
    paintGameIcon(iconPainter, QRect(0, 0, 64, 64), game.initials, game.tint);
    iconPainter.end();
    iconLabel->setPixmap(icon);
  }
}

void GamePage::updateStatus()
{
  if (game.isInstalled) {
    statusTitle->setText(QStringLiteral("Ready to play"));
    statusDetail->setText(QStringLiteral("%1 · %2").arg(realmChoice->currentText(), accountChoice->currentText()));
    primaryButton->setText(QStringLiteral("Play"));
    primaryButton->setEnabled(true);
    primaryButton->setToolTip(QString());
  } else {
    statusTitle->setText(QStringLiteral("Not installed"));
    statusDetail->setText(QStringLiteral("Required compatibility installs with the game."));
    primaryButton->setText(QStringLiteral("Install"));
    primaryButton->setEnabled(false);
    primaryButton->setToolTip(QStringLiteral("Installing games is not built yet."));
  }
}

void GamePage::showGame(const Game& newGame)
{
  game = newGame;
  titleLabel->setText(game.title);
  subtitleLabel->setText(game.build.isEmpty() ? game.edition : QStringLiteral("%1 · %2").arg(game.edition, game.build));
  realmChoice->clear();
  accountChoice->clear();
  if (game.isInstalled) {
    realmChoice->addItems(game.realms);
    accountChoice->addItems(game.accounts);
  } else {
    realmChoice->addItem(QStringLiteral("Install the game first"));
    accountChoice->addItem(QStringLiteral("Install the game first"));
  }
  realmChoice->setEnabled(game.isInstalled);
  accountChoice->setEnabled(game.isInstalled);
  installNote->setVisible(!game.isInstalled);
  installedSection->setVisible(game.isInstalled);
  characterSection->setVisible(game.isInstalled && game.recentCharacter.has_value());
  if (game.recentCharacter) {
    characterName->setText(game.recentCharacter->name);
    characterSummary->setText(game.recentCharacter->summary);
  }
  tabs->setCurrentIndex(0);
  refreshIcons();
  updateStatus();
}
