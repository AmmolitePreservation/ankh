#pragma once

#include "ammoliteClient.h"
#include "gameCatalog.h"
#include "palette.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QPushButton;
class QStackedWidget;
class QTabBar;
class QVBoxLayout;

class GamePage : public QWidget {
  Q_OBJECT

public:
  explicit GamePage(QWidget* parent = nullptr);

  void showGame(const Game& game);
  void setDownloads(int serverID, const QList<DownloadGroup>& groups);
  void setDownloadsFailed(int serverID, const QString& message);
  void setGameAccounts(const QList<AccountDomain>& domains);
  void setGameAccountsFailed(const QString& message);
  void applyPalette(const Palette& newColors);

private:
  void refreshIcons();
  void refreshAccounts();
  void updateStatus();
  void clearDownloads();

  Palette colors;
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
  QWidget* downloadsBox;
  QVBoxLayout* downloadsLayout;
  QLabel* upToDateIcon;
  QLabel* compatibilityIcon;
  QLabel* optionalToolsIcon;
  QPushButton* checkForUpdates;
  QPushButton* clientToolsLink;
  QLabel* statusDot;
  QLabel* statusTitle;
  QLabel* statusDetail;
  QPushButton* primaryButton;
  QList<AccountDomain> accountDomains;
  QString accountsProblem;
};
