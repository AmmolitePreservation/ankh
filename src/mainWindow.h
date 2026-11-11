#pragma once

#include "ammoliteClient.h"
#include "credentialStore.h"
#include "gameCatalog.h"
#include "installJob.h"
#include "installLibrary.h"
#include "palette.h"
#include "updateOffer.h"

#include <QMap>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QWidget>

class EdgeGrain;
class GamePage;
class LibraryPane;
class QAction;
class QPushButton;
class QStackedWidget;
class QSvgWidget;
class SettingsPage;
class SettingsPane;
class SignInDialog;
class UpdateSheet;
class UserMenuButton;
class WindowControls;

class SplitHandle : public QWidget {
  Q_OBJECT

public:
  explicit SplitHandle(QWidget* parent = nullptr);

  void setLineColor(const QColor& color);

signals:
  void dragged(int globalX);

protected:
  void paintEvent(QPaintEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

private:
  QColor lineColor;
};

class MainWindow : public QWidget {
  Q_OBJECT

public:
  MainWindow();

protected:
  bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
  void showEvent(QShowEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void applyPalette();
  void showLibrary();
  void showSettings(int section);
  void enterSignedInState(const SignedInUser& user);
  void enterSignedOutState(const QString& note);
  void checkManifest();
  void applyManifest(const Manifest& manifest, const QByteArray& etag);
  void showCollection(const Manifest& manifest);
  void showCollectionProblem(const QString& message);
  void openSignIn(const QString& note, bool isCreatingAccount);
  void setSidebarWidth(int width);
  void placeWindowControls();
  void openUpdateSheet();
  QString libraryRoot() const;
  QStringList rememberedInstalls() const;
  void rememberInstalls(const QStringList& folders);
  void rescanLibrary();
  void rebuildGames();
  const Receipt* receiptFor(const Game& game) const;
  void refreshInstallState();
  void startInstall(const Game& game);
  void startUpdate(const Game& game);
  void startAttach(const Game& game);
  void refreshAttachedChoices();
  void startLaunch(const Game& game, const QString& world, const QString& username);
  void startPluginChange(const Game& game, const QStringList& optionalPluginNames);
  void refreshPluginState();
  void runPlan(const Game& game, const InstallPlan& plan);
  void finishInstall(const QString& note);
  void chooseLibraryRoot();
  long hitTest(const QPoint& position) const;

  enum class LibraryAction { signIn, retry };

  QSettings settings;
  AmmoliteClient* client;
  CredentialStore tokenStore;
  SignInDialog* signInDialog;
  WindowControls* windowControls;
  EdgeGrain* edgeGrain;
  QWidget* sidebar;
  SplitHandle* handle;
  QSvgWidget* wordmarkImage;
  QStackedWidget* leftStack;
  QStackedWidget* mainStack;
  LibraryPane* libraryPane;
  SettingsPane* settingsPane;
  GamePage* gamePage;
  SettingsPage* settingsPage;
  QPushButton* libraryNavigation;
  QPushButton* settingsButton;
  QPushButton* activityButton;
  UserMenuButton* userMenuButton;
  QAction* signInAction;
  QAction* signOutAction;
  QTimer updateTimer;
  QByteArray manifestEtag;
  QString updateStatus;
  UpdateOffer pendingOffer;
  UpdateSheet* openSheet = nullptr;
  LibraryAction libraryAction = LibraryAction::signIn;
  bool hasSelectedGame = false;
  bool hasCollection = false;
  QList<World> worlds;
  QList<Package> packages;
  QList<AttachedGame> attachedGames;
  LibraryScan libraryScan;
  Game shownGame{};
  InstallJob* installJob = nullptr;
  class TurbineLauncher* launcher = nullptr;
  QMap<QString, QStringList> attachedWorlds;
  QMap<QString, QString> attachedWorldProblems;
  QString launchingGameId;
  QString listingWorldsGameId;
  QString installingGameId;
  QString installingLabel;
  QString installStepText;
  qint64 installDone = 0;
  qint64 installTotal = 0;
  QMap<QString, QString> installNotes;
  QSize desiredSize;
  bool isSizeApplied = false;
  bool isMaximizePressed = false;
};
