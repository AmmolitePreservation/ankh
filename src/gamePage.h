#pragma once

#include "gameCatalog.h"
#include "skin.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTabBar;

class GamePage : public QWidget {
  Q_OBJECT

public:
  explicit GamePage(QWidget* parent = nullptr);

  void showGame(const Game& game);
  void applySkin(const Skin& skin);

private:
  void refreshIcons();
  void updateStatus();

  Skin skin;
  Game game;
  QLabel* iconLabel;
  QLabel* titleLabel;
  QLabel* subtitleLabel;
  QPushButton* gameOptions;
  QTabBar* tabs;
  QStackedWidget* pages;
  QComboBox* realmChoice;
  QComboBox* accountChoice;
  QPushButton* manageAccounts;
  QPushButton* launchSettingsToggle;
  QLabel* launchSettingsNote;
  QWidget* installedSection;
  QWidget* characterSection;
  QLabel* installNote;
  QLabel* upToDateIcon;
  QLabel* compatibilityIcon;
  QLabel* optionalToolsIcon;
  QLabel* portraitLabel;
  QLabel* characterName;
  QLabel* characterSummary;
  QPushButton* checkForUpdates;
  QPushButton* clientToolsLink;
  QLabel* statusDot;
  QLabel* statusTitle;
  QLabel* statusDetail;
  QPushButton* primaryButton;
};
