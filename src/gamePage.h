#pragma once

#include "ammoliteClient.h"
#include "gameCatalog.h"
#include "palette.h"

#include <QWidget>

class QComboBox;
class QLabel;
class QProgressBar;
class QPushButton;
class QStackedWidget;
class QTabBar;
class QVBoxLayout;
class ToolsPane;

enum class InstallState { notInstalled, unavailable, installing, installed, updateAvailable, busyElsewhere };

class GamePage : public QWidget {
  Q_OBJECT

public:
  explicit GamePage(QWidget* parent = nullptr);

  void showGame(const Game& game);
  void refreshGame(const Game& game);
  void setPackages(const QList<Package>& packages);
  void setInstallState(InstallState state, const QString& detail);
  void showInstallProgress(const QString& text, qint64 done, qint64 total);
  void setGameAccounts(const QList<AccountDomain>& domains);
  void setGameAccountsFailed(const QString& message);
  // What the Client tools tab shows: whether the game is installed, its installed tools, and why tools cannot change now.
  void setToolState(bool isInstalled, const QStringList& installedNames, const QString& busyReason);
  QStringList chosenOptionalTools() const;
  void showScreenshot(const QString& packageName, const QByteArray& image);
  void showScreenshotProblem(const QString& packageName, const QString& message);
  void applyPalette(const Palette& newColors);

signals:
  void installRequested(const Game& game);
  void updateRequested(const Game& game);
  void updateCheckRequested();
  void cancelRequested();
  void toolsApplyRequested(const Game& game, const QStringList& optionalToolNames);
  void toolChoiceChanged();
  void screenshotWanted(const Package& package);

private:
  void refreshIcons();
  void refreshAccounts();
  void updateStatus();
  void clearDownloads();
  void showDownloads();
  void refreshTools();

  Palette colors;
  Game game;
  QList<Package> packages;
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
  QLabel* upToDateLabel;
  QLabel* compatibilityIcon;
  QLabel* optionalToolsIcon;
  QLabel* optionalToolsSummary;
  ToolsPane* toolsPane;
  bool toolsInstalled = false;
  QStringList toolsInstalledNames;
  QString toolsBusyReason;
  QPushButton* checkForUpdates;
  QPushButton* clientToolsLink;
  QLabel* statusDot;
  QLabel* statusTitle;
  QLabel* statusDetail;
  QProgressBar* progress;
  QPushButton* primaryButton;
  QList<AccountDomain> accountDomains;
  QString accountsProblem;
  InstallState installState = InstallState::notInstalled;
  QString installDetail;
};
