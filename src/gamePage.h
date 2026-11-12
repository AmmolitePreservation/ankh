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
class PluginsPane;

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
  // What the Plugins tab shows: whether the game is installed, its installed plugins, and why plugins cannot change now.
  void setPluginState(bool isInstalled, const QStringList& installedNames, const QString& busyReason);
  void setResearchPluginsShown(bool isShown);
  QStringList chosenOptionalPlugins() const;
  void showScreenshot(const QString& packageName, const QByteArray& image);
  void showScreenshotProblem(const QString& packageName, const QString& message);
  // An attached game's worlds (from its own launcher service) and the logins kept on this computer, with the ones
  // to preselect; a problem replaces the world list.
  void setAttachedWorlds(const QStringList& worlds, const QString& selected, const QString& problem);
  void setLocalLogins(const QStringList& usernames, const QString& selected);
  // An attached game that Steam starts has neither: the two choices say so and Play needs nothing chosen.
  void showSteamStart();
  // While a launch runs the Play button is held and the status line shows the step; an empty text ends it.
  void showLaunchStatus(const QString& text);
  QString selectedRealm() const;
  QString selectedAccount() const;
  void applyPalette(const Palette& newColors);

signals:
  void installRequested(const Game& game);
  void updateRequested(const Game& game);
  void updateCheckRequested();
  void cancelRequested();
  void pluginsApplyRequested(const Game& game, const QStringList& optionalPluginNames);
  void pluginChoiceChanged();
  void screenshotWanted(const Package& package);
  void launchRequested(const Game& game, const QString& world, const QString& username);
  void manageLoginsRequested(const Game& game);
  void realmChosen(const Game& game, const QString& world);
  void accountChosen(const Game& game, const QString& username);

private:
  void refreshIcons();
  void refreshAccounts();
  void updateStatus();
  void clearDownloads();
  void showDownloads();
  void refreshPlugins();

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
  QLabel* accountLabel;
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
  QLabel* optionalPluginsIcon;
  QLabel* optionalPluginsSummary;
  PluginsPane* pluginsPane;
  bool pluginsInstalled = false;
  QStringList pluginsInstalledNames;
  QString pluginsBusyReason;
  bool researchPluginsShown = false;
  QPushButton* checkForUpdates;
  QPushButton* pluginsLink;
  QLabel* statusDot;
  QLabel* statusTitle;
  QLabel* statusDetail;
  QProgressBar* progress;
  QPushButton* primaryButton;
  QList<AccountDomain> accountDomains;
  QString accountsProblem;
  InstallState installState = InstallState::notInstalled;
  QString installDetail;
  QString launchStatus;
  bool isFillingChoices = false;
};
