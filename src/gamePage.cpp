#include "gamePage.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

GamePage::GamePage(QWidget* parent)
  : QWidget(parent),
    iconLabel(new QLabel(this)),
    titleLabel(new QLabel(this)),
    editionLabel(new QLabel(this)),
    aboutLabel(new QLabel(this)),
    statusLabel(new QLabel(this)),
    primaryButton(new QPushButton(QStringLiteral("Install"), this)),
    tabs(new QTabBar(this)),
    pages(new QStackedWidget(this))
{
  iconLabel->setFixedSize(48, 48);
  titleLabel->setProperty("heading", true);
  editionLabel->setProperty("secondary", true);
  aboutLabel->setProperty("secondary", true);
  aboutLabel->setWordWrap(true);
  aboutLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
  statusLabel->setProperty("secondary", true);
  primaryButton->setObjectName(QStringLiteral("primary"));
  primaryButton->setEnabled(false);
  primaryButton->setToolTip(QStringLiteral("Installing games is not available in this build."));
  tabs->addTab(QStringLiteral("Play"));
  tabs->addTab(QStringLiteral("About"));
  tabs->setExpanding(false);
  tabs->setDrawBase(false);

  auto* identity = new QVBoxLayout;
  identity->setSpacing(0);
  identity->addWidget(titleLabel);
  identity->addWidget(editionLabel);

  auto* header = new QHBoxLayout;
  header->setSpacing(16);
  header->addWidget(iconLabel);
  header->addLayout(identity, 1);

  auto* playPage = new QWidget(this);
  auto* playLayout = new QVBoxLayout(playPage);
  playLayout->setContentsMargins(0, 24, 0, 0);
  playLayout->setSpacing(16);
  auto* form = new QFormLayout;
  form->setHorizontalSpacing(16);
  form->setVerticalSpacing(12);
  form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  auto* realm = new QComboBox(playPage);
  realm->addItem(QStringLiteral("No realms configured"));
  realm->setEnabled(false);
  realm->setAccessibleName(QStringLiteral("Realm"));
  realm->setMinimumWidth(320);
  auto* account = new QComboBox(playPage);
  account->addItem(QStringLiteral("No accounts"));
  account->setEnabled(false);
  account->setAccessibleName(QStringLiteral("Account"));
  account->setMinimumWidth(320);
  form->addRow(QStringLiteral("Realm"), realm);
  form->addRow(QStringLiteral("Account"), account);
  playLayout->addLayout(form);
  playLayout->addStretch(1);

  auto* aboutPage = new QWidget(this);
  auto* aboutLayout = new QVBoxLayout(aboutPage);
  aboutLayout->setContentsMargins(0, 24, 0, 0);
  aboutLayout->addWidget(aboutLabel);
  aboutLayout->addStretch(1);

  pages->addWidget(playPage);
  pages->addWidget(aboutPage);

  auto* body = new QWidget(this);
  auto* bodyLayout = new QVBoxLayout(body);
  bodyLayout->setContentsMargins(32, 28, 32, 16);
  bodyLayout->setSpacing(16);
  bodyLayout->addLayout(header);
  bodyLayout->addWidget(tabs);
  bodyLayout->addWidget(pages, 1);

  auto* bottomBar = new QWidget(this);
  bottomBar->setObjectName(QStringLiteral("bottomBar"));
  bottomBar->setFixedHeight(76);
  auto* bottomLayout = new QHBoxLayout(bottomBar);
  bottomLayout->setContentsMargins(32, 0, 32, 0);
  bottomLayout->addWidget(statusLabel, 1);
  bottomLayout->addWidget(primaryButton);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(body, 1);
  layout->addWidget(bottomBar);

  connect(tabs, &QTabBar::currentChanged, pages, &QStackedWidget::setCurrentIndex);
}

void GamePage::showGame(const Game& game)
{
  const qreal ratio = devicePixelRatioF();
  QPixmap icon(QSize(48, 48) * ratio);
  icon.setDevicePixelRatio(ratio);
  icon.fill(Qt::transparent);
  QPainter painter(&icon);
  paintGameIcon(painter, QRect(0, 0, 48, 48), game.initials, game.tint);
  painter.end();
  iconLabel->setPixmap(icon);
  titleLabel->setText(game.title);
  editionLabel->setText(game.edition);
  aboutLabel->setText(QStringLiteral("%1 - %2").arg(game.title, game.edition));
  statusLabel->setText(game.isInstalled ? QStringLiteral("Ready to play") : QStringLiteral("Not installed"));
  tabs->setCurrentIndex(0);
}
