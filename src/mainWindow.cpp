#include "mainWindow.h"

#include "accountView.h"
#include "appVersion.h"
#include "captionButton.h"
#include "edgeGrain.h"
#include "fatal.h"
#include "gameCatalog.h"
#include "gamePage.h"
#include "icons.h"
#include "installSheet.h"
#include "libraryPane.h"
#include "settingsPage.h"
#include "settingsPane.h"
#include "signInDialog.h"
#include "updateOffer.h"
#include "updateSheet.h"
#include "userMenuButton.h"
#include "windowControls.h"

#include <QAbstractScrollArea>
#include <QFrame>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QSvgWidget>
#include <QSysInfo>
#include <QVBoxLayout>
#include <QWindow>

#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

namespace {

constexpr int minimumSidebarWidth = 300;
constexpr int maximumSidebarWidth = 420;
constexpr int defaultSidebarWidth = 360;
constexpr int headerHeight = 84;
constexpr int resizeBorder = 6;
const QString signedOutName = QStringLiteral("Sign in");
constexpr const char* defaultServiceAddress = "https://ammolite.unseencolor.com:1996";

QUrl serviceUrl(const QSettings& settings)
{
  return validatedServiceUrl(settings.value(QStringLiteral("ammolite/url"), QString::fromLatin1(defaultServiceAddress)).toString());
}

QString libraryRootFrom(const QSettings& settings)
{
  const QString root = settings.value(QStringLiteral("library/root"), defaultLibraryRoot()).toString();
  const QString problem = libraryRootProblem(root);
  if (!problem.isEmpty()) {
    fatal(QStringLiteral("%1 (%2)").arg(problem, QDir::toNativeSeparators(settings.fileName())));
  }
  return root;
}

}

SplitHandle::SplitHandle(QWidget* parent)
  : QWidget(parent)
{
  setFixedWidth(5);
  setCursor(Qt::SizeHorCursor);
}

void SplitHandle::setLineColor(const QColor& color)
{
  lineColor = color;
  update();
}

void SplitHandle::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.fillRect(rect(), palette().window());
  painter.fillRect(QRect(0, 0, 1, height()), lineColor);
}

void SplitHandle::mouseMoveEvent(QMouseEvent* event)
{
  if (event->buttons() & Qt::LeftButton) {
    emit dragged(event->globalPosition().toPoint().x());
  }
}

MainWindow::MainWindow()
  : settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("AmmolitePreservation"), QStringLiteral("Ankh")),
    client(new AmmoliteClient(serviceUrl(settings), QString::fromLatin1(appVersion), this)),
    tokenStore(QStringLiteral("Ankh/AmmolitePreservation/apiToken")),
    signInDialog(new SignInDialog(this)),
    windowControls(new WindowControls(this)),
    edgeGrain(new EdgeGrain(this)),
    sidebar(new QWidget(this)),
    handle(new SplitHandle(this)),
    wordmarkImage(new QSvgWidget(QStringLiteral(":/ankhWordmarkGold.svg"), this)),
    leftStack(new QStackedWidget(this)),
    mainStack(new QStackedWidget(this)),
    libraryPane(new LibraryPane(settings, this)),
    settingsPane(new SettingsPane(this)),
    gamePage(new GamePage(this)),
    settingsPage(new SettingsPage(serviceUrl(settings).toString(), libraryRootFrom(settings), this)),
    libraryNavigation(new QPushButton(QStringLiteral("Library"), this)),
    settingsButton(new QPushButton(QStringLiteral("Settings"), this)),
    activityButton(new QPushButton(QStringLiteral("Activity"), this)),
    userMenuButton(new UserMenuButton(signedOutName, this)),
    signInAction(new QAction(QStringLiteral("Sign in"), this)),
    signOutAction(new QAction(QStringLiteral("Sign out"), this))
{
  setWindowTitle(QStringLiteral("Ankh"));
  setWindowIcon(QIcon(QStringLiteral(":/ankh.ico")));
  setMinimumSize(1180, 760);
  desiredSize = settings.value(QStringLiteral("window/size"), QSize(1600, 1000)).toSize();
  resize(desiredSize);
  winId();

  sidebar->setObjectName(QStringLiteral("sidebar"));
  auto* sidebarHeader = new QWidget(sidebar);
  sidebarHeader->setObjectName(QStringLiteral("sidebarHeader"));
  sidebarHeader->setFixedHeight(headerHeight);
  wordmarkImage->setFixedSize(198, 51);
  wordmarkImage->setAccessibleName(QStringLiteral("Ankh"));
  auto* sidebarHeaderLayout = new QHBoxLayout(sidebarHeader);
  sidebarHeaderLayout->setContentsMargins(26, 0, 20, 0);
  sidebarHeaderLayout->addWidget(wordmarkImage);
  sidebarHeaderLayout->addStretch(1);

  leftStack->addWidget(libraryPane);
  leftStack->addWidget(settingsPane);

  settingsButton->setProperty("footer", true);
  settingsButton->setIconSize(QSize(22, 22));
  activityButton->setIconSize(QSize(22, 22));
  activityButton->setProperty("footer", true);
  activityButton->setEnabled(false);
  activityButton->setToolTip(QStringLiteral("Activity is not built yet."));
  auto* foot = new QWidget(sidebar);
  foot->setObjectName(QStringLiteral("sidebarFoot"));
  foot->setFixedHeight(100);
  auto* footDivider = new QFrame(foot);
  footDivider->setObjectName(QStringLiteral("divider"));
  footDivider->setFixedHeight(1);
  auto* footLine = new QHBoxLayout;
  footLine->setContentsMargins(24, 0, 24, 0);
  footLine->addWidget(footDivider);
  auto* footRow = new QHBoxLayout;
  footRow->setContentsMargins(26, 0, 24, 0);
  footRow->setSpacing(56);
  footRow->addWidget(settingsButton);
  footRow->addWidget(activityButton);
  footRow->addStretch(1);
  auto* footLayout = new QVBoxLayout(foot);
  footLayout->setContentsMargins(0, 0, 0, 0);
  footLayout->setSpacing(0);
  footLayout->addLayout(footLine);
  footLayout->addLayout(footRow, 1);

  auto* sidebarLayout = new QVBoxLayout(sidebar);
  sidebarLayout->setContentsMargins(0, 0, 0, 0);
  sidebarLayout->setSpacing(0);
  sidebarLayout->addWidget(sidebarHeader);
  sidebarLayout->addWidget(leftStack, 1);
  sidebarLayout->addWidget(foot);

  libraryNavigation->setProperty("navigation", true);
  libraryNavigation->setCheckable(true);
  libraryNavigation->setChecked(true);
  libraryNavigation->setFixedHeight(headerHeight - 16);
  auto* mainHeader = new QWidget(this);
  mainHeader->setObjectName(QStringLiteral("header"));
  mainHeader->setFixedHeight(headerHeight);
  auto* mainHeaderLayout = new QHBoxLayout(mainHeader);
  mainHeaderLayout->setContentsMargins(22, 10, 22, 6);
  mainHeaderLayout->setSpacing(8);
  mainHeaderLayout->addWidget(libraryNavigation);
  mainHeaderLayout->addStretch(1);
  mainHeaderLayout->addWidget(userMenuButton, 0, Qt::AlignBottom);

  auto* emptyLibrary = new QLabel(QStringLiteral("Select a game"), this);
  emptyLibrary->setProperty("secondary", true);
  emptyLibrary->setAlignment(Qt::AlignCenter);
  mainStack->addWidget(emptyLibrary);
  mainStack->addWidget(gamePage);
  mainStack->addWidget(settingsPage);

  auto* mainColumn = new QVBoxLayout;
  mainColumn->setContentsMargins(0, 0, 0, 0);
  mainColumn->setSpacing(0);
  mainColumn->addWidget(mainHeader);
  mainColumn->addWidget(mainStack, 1);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(sidebar);
  layout->addWidget(handle);
  layout->addLayout(mainColumn, 1);

  setSidebarWidth(settings.value(QStringLiteral("window/sidebarWidth"), defaultSidebarWidth).toInt());
  applyPalette();
  placeWindowControls();

  auto* userMenu = new QMenu(userMenuButton);
  signOutAction->setVisible(false);
  userMenu->addAction(signInAction);
  userMenu->addAction(signOutAction);
  userMenu->addSeparator();
  connect(signInAction, &QAction::triggered, this, [this] { openSignIn(QString(), false); });
  connect(signOutAction, &QAction::triggered, client, &AmmoliteClient::signOut);
  connect(userMenu->addAction(QStringLiteral("Settings")), &QAction::triggered, this, [this] { showSettings(0); });
  connect(userMenu->addAction(QStringLiteral("Attributions")), &QAction::triggered, this, [this] { showSettings(3); });
  connect(userMenuButton, &QPushButton::clicked, this, [this, userMenu] {
    userMenu->exec(userMenuButton->mapToGlobal(QPoint(userMenuButton->width() - userMenu->sizeHint().width(), userMenuButton->height())));
  });

  connect(windowControls, &WindowControls::minimizeRequested, this, &QWidget::showMinimized);
  connect(windowControls, &WindowControls::maximizeRequested, this, [this] {
    if (isMaximized()) {
      showNormal();
    } else {
      showMaximized();
    }
  });
  connect(windowControls, &WindowControls::closeRequested, this, &QWidget::close);
  connect(windowControls, &WindowControls::resized, this, &MainWindow::placeWindowControls);
  connect(handle, &SplitHandle::dragged, this, [this](int globalX) {
    setSidebarWidth(globalX - mapToGlobal(QPoint(0, 0)).x());
  });
  connect(libraryNavigation, &QPushButton::clicked, this, &MainWindow::showLibrary);
  connect(settingsButton, &QPushButton::clicked, this, [this] { showSettings(0); });
  connect(settingsPane, &SettingsPane::sectionSelected, settingsPage, &SettingsPage::showSection);
  connect(settingsPage, &SettingsPage::updateCheckRequested, this, &MainWindow::checkManifest);
  connect(settingsPage, &SettingsPage::libraryRootChangeRequested, this, &MainWindow::chooseLibraryRoot);
  connect(gamePage, &GamePage::installRequested, this, &MainWindow::startInstall);
  connect(gamePage, &GamePage::updateRequested, this, &MainWindow::startUpdate);
  connect(gamePage, &GamePage::updateCheckRequested, this, &MainWindow::checkManifest);
  connect(gamePage, &GamePage::cancelRequested, this, [this] {
    if (installJob != nullptr) {
      installJob->cancel();
    }
  });
  connect(libraryPane, &LibraryPane::gameSelected, this, [this](const Game& game) {
    hasSelectedGame = true;
    if (game.id == shownGame.id) {
      shownGame = game;
      gamePage->refreshGame(game);
    } else {
      shownGame = game;
      gamePage->showGame(game);
    }
    refreshInstallState();
    settings.setValue(QStringLiteral("library/selected"), game.id);
    if (leftStack->currentWidget() == libraryPane) {
      showLibrary();
    }
  });
  connect(libraryPane, &LibraryPane::actionRequested, this, [this] {
    if (libraryAction == LibraryAction::signIn) {
      openSignIn(QString(), false);
    } else {
      libraryPane->showMessage(QStringLiteral("Loading the collection..."), QString());
      client->loadMe();
    }
  });

  AccountView* accountView = settingsPage->accountView();
  connect(accountView, &AccountView::signInRequested, this, [this] { openSignIn(QString(), false); });
  connect(accountView, &AccountView::createAccountRequested, this, [this] { openSignIn(QString(), true); });
  connect(accountView, &AccountView::signOutRequested, client, &AmmoliteClient::signOut);
  connect(signInDialog, &SignInDialog::signInRequested, this, [this](const QString& username, const QString& password) {
    client->signIn(username, password, QStringLiteral("Ankh on %1").arg(QSysInfo::machineHostName()));
  });
  connect(signInDialog, &SignInDialog::createAccountRequested, this,
    [this](const QString& inviteCode, const QString& username, const QString& password, const QString& confirmation) {
      client->registerAccount(inviteCode, username, password, confirmation, QStringLiteral("Ankh on %1").arg(QSysInfo::machineHostName()));
    });
  connect(client, &AmmoliteClient::signedIn, this, [this](const SignedInUser& user, const QString& token) {
    tokenStore.save(token);
    signInDialog->reset();
    signInDialog->accept();
    enterSignedInState(user);
  });
  connect(client, &AmmoliteClient::signInFailed, signInDialog, &SignInDialog::showFailure);
  connect(client, &AmmoliteClient::registerFailed, signInDialog, &SignInDialog::showFailure);
  connect(client, &AmmoliteClient::signedOut, this, [this](const QString& warning) {
    tokenStore.remove();
    enterSignedOutState(warning);
  });
  connect(client, &AmmoliteClient::sessionExpired, this, [this] {
    tokenStore.remove();
    const QString note = QStringLiteral("Your sign-in has ended. Sign in again.");
    enterSignedOutState(note);
    openSignIn(note, false);
  });
  connect(client, &AmmoliteClient::meLoaded, this, &MainWindow::enterSignedInState);
  connect(client, &AmmoliteClient::manifestLoaded, this, &MainWindow::applyManifest);
  connect(client, &AmmoliteClient::manifestUnchanged, this, [this] { settingsPage->setUpdateStatus(updateStatus); });
  connect(client, &AmmoliteClient::gameAccountsLoaded, gamePage, &GamePage::setGameAccounts);
  connect(client, &AmmoliteClient::requestFailed, this, [this](const QString& what, const QString& message) {
    if (what == QStringLiteral("game accounts")) {
      gamePage->setGameAccountsFailed(message);
      return;
    }
    if (what == QStringLiteral("manifest")) {
      updateStatus = QStringLiteral("Could not check for updates: %1").arg(message);
      settingsPage->setUpdateStatus(updateStatus);
      if (!signOutAction->isVisible() || hasCollection) {
        return;
      }
    }
    showCollectionProblem(QStringLiteral("Could not load the collection: %1").arg(message));
  });
  connect(windowControls->updateButton(), &QAbstractButton::clicked, this, &MainWindow::openUpdateSheet);

  updateTimer.setInterval(15 * 60 * 1000);
  connect(&updateTimer, &QTimer::timeout, this, &MainWindow::checkManifest);
  updateTimer.start();
  rescanLibrary();

  const std::optional<QString> token = tokenStore.load();
  if (token) {
    client->setToken(*token);
    libraryPane->showMessage(QStringLiteral("Signing in..."), QString());
    client->loadMe();
  } else {
    enterSignedOutState(QString());
    QTimer::singleShot(0, this, [this] { openSignIn(QString(), false); });
  }
}

void MainWindow::openSignIn(const QString& note, bool isCreatingAccount)
{
  if (signInDialog->isVisible()) {
    return;
  }
  if (isCreatingAccount) {
    signInDialog->showCreateAccount();
  } else {
    signInDialog->showSignIn(note);
  }
  signInDialog->open();
}

void MainWindow::enterSignedInState(const SignedInUser& user)
{
  settingsPage->accountView()->showSignedIn(user);
  userMenuButton->setUserName(user.username);
  signInAction->setVisible(false);
  signOutAction->setVisible(true);
  libraryPane->showMessage(QStringLiteral("Loading the collection..."), QString());
  hasCollection = false;
  manifestEtag.clear();
  checkManifest();
  client->loadGameAccounts();
}

void MainWindow::enterSignedOutState(const QString& note)
{
  client->setToken(QString());
  settingsPage->accountView()->showSignedOut(note);
  userMenuButton->setUserName(signedOutName);
  signInAction->setVisible(true);
  signOutAction->setVisible(false);
  hasSelectedGame = false;
  hasCollection = false;
  shownGame = Game{};
  worlds.clear();
  packages.clear();
  manifestEtag.clear();
  gamePage->setPackages({});
  libraryAction = LibraryAction::signIn;
  QTimer::singleShot(0, this, &MainWindow::checkManifest);
  libraryPane->showMessage(note.isEmpty() ? QStringLiteral("Sign in to see the collection.") : note, QStringLiteral("Sign in"));
  if (leftStack->currentWidget() == libraryPane) {
    showLibrary();
  }
}

void MainWindow::checkManifest()
{
  settingsPage->setUpdateStatus(QStringLiteral("Checking for updates..."));
  client->loadManifest(manifestEtag);
}

void MainWindow::applyManifest(const Manifest& manifest, const QByteArray& etag)
{
  manifestEtag = etag;
  if (!manifest.ankh) {
    updateStatus = QStringLiteral("Could not check for updates: %1").arg(manifest.ankhProblem);
  } else {
    const ReleaseCheck check = checkRelease(*manifest.ankh, QString::fromLatin1(appVersion));
    if (!check.problem.isEmpty()) {
      updateStatus = QStringLiteral("Could not check for updates: %1").arg(check.problem);
    } else if (check.update) {
      pendingOffer = *check.update;
      if (openSheet != nullptr) {
        openSheet->setOffer(pendingOffer);
      }
      windowControls->updateButton()->setToolTip(QStringLiteral("Update available: %1").arg(pendingOffer.version));
      windowControls->updateButton()->setVisible(true);
      updateStatus = QStringLiteral("Update available: %1").arg(pendingOffer.version);
    } else {
      windowControls->updateButton()->setVisible(false);
      updateStatus = QStringLiteral("Ankh is up to date.");
    }
  }
  settingsPage->setUpdateStatus(updateStatus);
  if (manifest.hasCollection) {
    showCollection(manifest.worlds, manifest.packages);
  }
}

void MainWindow::showCollection(const QList<World>& newWorlds, const QList<Package>& newPackages)
{
  worlds = newWorlds;
  packages = newPackages;
  rebuildGames();
}

void MainWindow::rebuildGames()
{
  QList<Game> games;
  for (const World& world : worlds) {
    Game game = gameFromWorld(world);
    game.isInstalled = receiptFor(game) != nullptr;
    games.append(game);
  }
  gamePage->setPackages(packages);
  libraryPane->showGames(games);
  if (!hasCollection) {
    libraryPane->selectGame(settings.value(QStringLiteral("library/selected")).toString());
  }
  hasCollection = true;
  refreshInstallState();
}

const Receipt* MainWindow::receiptFor(const Game& game) const
{
  for (const Package& package : packagesFor(game, packages)) {
    if (package.role == QLatin1String("client")) {
      return receiptForClient(libraryScan.receipts, package.name);
    }
  }
  return nullptr;
}

QString MainWindow::libraryRoot() const
{
  return libraryRootFrom(settings);
}

QStringList MainWindow::rememberedInstalls() const
{
  return settings.value(QStringLiteral("library/installs")).toStringList();
}

// Every install folder is remembered, so a game put somewhere else, or installed before the library folder
// changed, is still known after a restart.
void MainWindow::rememberInstalls(const QStringList& folders)
{
  QStringList remembered = rememberedInstalls();
  for (const QString& folder : folders) {
    if (!remembered.contains(folder, Qt::CaseInsensitive)) {
      remembered.append(folder);
    }
  }
  settings.setValue(QStringLiteral("library/installs"), remembered);
}

void MainWindow::rescanLibrary()
{
  QStringList remembered;
  for (const QString& folder : rememberedInstalls()) {
    if (QFile::exists(receiptPath(folder))) {
      remembered.append(folder);
    }
  }
  if (remembered != rememberedInstalls()) {
    settings.setValue(QStringLiteral("library/installs"), remembered);
  }
  libraryScan = scanLibrary(libraryRoot(), remembered);
  settingsPage->setLibraryProblems(libraryScan.problems);
  if (hasCollection) {
    rebuildGames();
  }
}

void MainWindow::refreshInstallState()
{
  if (shownGame.id.isEmpty()) {
    return;
  }
  if (installJob != nullptr) {
    if (shownGame.id == installingGameId) {
      gamePage->setInstallState(InstallState::installing, installStepText);
      gamePage->showInstallProgress(installStepText, installDone, installTotal);
    } else {
      gamePage->setInstallState(InstallState::busyElsewhere, QStringLiteral("Installing %1 first. One install runs at a time.").arg(installingLabel));
    }
    return;
  }
  const Receipt* receipt = receiptFor(shownGame);
  if (receipt != nullptr) {
    QString problem;
    const std::optional<InstallPlan> update = planUpdate(shownGame, packages, *receipt, problem);
    if (update) {
      QStringList changes;
      for (const Package& package : update->fetch) {
        changes.append(QStringLiteral("%1 %2").arg(package.label, package.latest->version));
      }
      for (const QString& name : update->removed) {
        changes.append(QStringLiteral("%1 removed").arg(name));
      }
      gamePage->setInstallState(InstallState::updateAvailable, QStringLiteral("Update available: %1").arg(changes.join(QStringLiteral(", "))));
    } else {
      gamePage->setInstallState(InstallState::installed, problem.isEmpty() ? installNotes.value(shownGame.id) : problem);
    }
    return;
  }
  QString problem;
  if (!planInstall(shownGame, packages, libraryRoot(), problem)) {
    gamePage->setInstallState(InstallState::unavailable, problem);
    return;
  }
  gamePage->setInstallState(InstallState::notInstalled, installNotes.value(shownGame.id));
}

void MainWindow::startInstall(const Game& game)
{
  QString problem;
  const std::optional<InstallPlan> plan = planInstall(game, packages, libraryRoot(), problem);
  if (!plan) {
    gamePage->setInstallState(InstallState::unavailable, problem);
    return;
  }
  runPlan(game, *plan);
}

void MainWindow::startUpdate(const Game& game)
{
  const Receipt* receipt = receiptFor(game);
  if (receipt == nullptr) {
    return;
  }
  QString problem;
  const std::optional<InstallPlan> plan = planUpdate(game, packages, *receipt, problem);
  if (!plan) {
    gamePage->setInstallState(InstallState::installed, problem);
    return;
  }
  runPlan(game, *plan);
}

void MainWindow::runPlan(const Game& game, const InstallPlan& startingPlan)
{
  if (installJob != nullptr) {
    return;
  }
  std::optional<InstallPlan> plan = startingPlan;
  InstallSheet sheet(game, *plan, this);
  connect(&sheet, &InstallSheet::destinationChosen, this, [this, &sheet, &plan, &game](const QString& parentFolder) {
    QString replanProblem;
    plan = planInstall(game, packages, parentFolder, replanProblem);
    if (!plan) {
      fatal(QStringLiteral("The install plan changed with its folder: %1").arg(replanProblem));
    }
    sheet.showPlan(*plan);
  });
  if (sheet.exec() != QDialog::Accepted) {
    return;
  }
  installNotes.remove(game.id);
  installingGameId = game.id;
  installingLabel = QStringLiteral("%1 %2").arg(game.title, game.edition);
  installStepText = QStringLiteral("Starting...");
  installDone = 0;
  installTotal = 0;
  installJob = new InstallJob(*plan, client->baseUrl(), client->token(), QString::fromLatin1(appVersion), this);
  connect(installJob, &InstallJob::statusChanged, this, [this](const QString& text) {
    installStepText = text;
    installDone = 0;
    installTotal = 0;
    if (shownGame.id == installingGameId) {
      gamePage->showInstallProgress(text, 0, 0);
    }
  });
  connect(installJob, &InstallJob::progressChanged, this, [this](qint64 done, qint64 total) {
    installDone = done;
    installTotal = total;
    if (shownGame.id == installingGameId) {
      gamePage->showInstallProgress(installStepText, done, total);
    }
  });
  connect(installJob, &InstallJob::finished, this, [this, isUpdate = plan->isUpdate()](const Receipt& receipt, const ApplySummary& summary) {
    if (!isUpdate) {
      rememberInstalls({receipt.folder});
      finishInstall(QString());
      return;
    }
    QString note = QStringLiteral("Updated to %1: %2 written, %3 deleted").arg(receipt.client.version)
      .arg(summary.written).arg(summary.deleted);
    if (!summary.kept.isEmpty()) {
      QStringList named = summary.kept.mid(0, 3);
      if (summary.kept.size() > 3) {
        named.append(QStringLiteral("%1 more").arg(summary.kept.size() - 3));
      }
      note += QStringLiteral(", kept as you had them: %1").arg(named.join(QStringLiteral(", ")));
    }
    finishInstall(note);
  });
  connect(installJob, &InstallJob::failed, this, &MainWindow::finishInstall);
  connect(installJob, &InstallJob::cancelled, this, [this] {
    finishInstall(QStringLiteral("The install was cancelled. Finished downloads are kept for next time."));
  });
  installJob->start();
  refreshInstallState();
}

void MainWindow::finishInstall(const QString& note)
{
  installJob->deleteLater();
  installJob = nullptr;
  if (note.isEmpty()) {
    installNotes.remove(installingGameId);
  } else {
    installNotes.insert(installingGameId, note);
  }
  installingGameId.clear();
  installingLabel.clear();
  rescanLibrary();
  refreshInstallState();
}

void MainWindow::chooseLibraryRoot()
{
  const QString chosen = QFileDialog::getExistingDirectory(this, QStringLiteral("Game library folder"), libraryRoot());
  if (chosen.isEmpty()) {
    return;
  }
  QStringList known;
  for (const Receipt& receipt : libraryScan.receipts) {
    known.append(receipt.folder);
  }
  rememberInstalls(known);
  settings.setValue(QStringLiteral("library/root"), QDir::toNativeSeparators(chosen));
  settingsPage->setLibraryRoot(libraryRoot());
  rescanLibrary();
  refreshInstallState();
}

void MainWindow::showCollectionProblem(const QString& message)
{
  hasSelectedGame = false;
  libraryAction = LibraryAction::retry;
  libraryPane->showMessage(message, QStringLiteral("Retry"));
  if (leftStack->currentWidget() == libraryPane) {
    showLibrary();
  }
}

void MainWindow::applyPalette()
{
  const Palette& colors = ankhPalette();
  QPalette applicationPalette = qApp->palette();
  applicationPalette.setColor(QPalette::Link, colors.accent);
  applicationPalette.setColor(QPalette::LinkVisited, colors.accent);
  qApp->setPalette(applicationPalette);
  qApp->setStyleSheet(buildStyleSheet(colors));
  handle->setLineColor(colors.border.lighter(130));
  windowControls->applyPalette(colors);
  libraryPane->applyPalette(colors);
  gamePage->applyPalette(colors);
  userMenuButton->setColors(colors.text, colors.secondary, colors.border);
  settingsButton->setIcon(makeIcon(IconKind::settings, 24, colors.text));
  activityButton->setIcon(makeIcon(IconKind::activity, 24, colors.text));
  QPalette handlePalette = handle->palette();
  handlePalette.setColor(QPalette::Window, colors.base);
  handle->setPalette(handlePalette);
}
void MainWindow::showLibrary()
{
  libraryNavigation->setChecked(true);
  leftStack->setCurrentWidget(libraryPane);
  mainStack->setCurrentIndex(hasSelectedGame ? 1 : 0);
}

void MainWindow::showSettings(int section)
{
  libraryNavigation->setChecked(false);
  leftStack->setCurrentWidget(settingsPane);
  settingsPane->selectSection(section);
  settingsPage->showSection(section);
  mainStack->setCurrentWidget(settingsPage);
}

void MainWindow::setSidebarWidth(int width)
{
  const int clamped = qBound(minimumSidebarWidth, width, maximumSidebarWidth);
  sidebar->setFixedWidth(clamped);
  settings.setValue(QStringLiteral("window/sidebarWidth"), clamped);
}

void MainWindow::placeWindowControls()
{
  edgeGrain->setGeometry(rect());
  edgeGrain->raise();
  windowControls->move(width() - windowControls->width(), 0);
  windowControls->raise();
}

void MainWindow::openUpdateSheet()
{
  UpdateSheet sheet(pendingOffer, QString::fromLatin1(appVersion), this);
  if (installJob != nullptr) {
    sheet.holdFor(QStringLiteral("The install of %1").arg(installingLabel));
  }
  openSheet = &sheet;
  sheet.exec();
  openSheet = nullptr;
}

long MainWindow::hitTest(const QPoint& position) const
{
  if (!isMaximized()) {
    const bool onLeft = position.x() < resizeBorder;
    const bool onRight = position.x() >= width() - resizeBorder;
    const bool onTop = position.y() < resizeBorder;
    const bool onBottom = position.y() >= height() - resizeBorder;
    if (onTop && onLeft) return HTTOPLEFT;
    if (onTop && onRight) return HTTOPRIGHT;
    if (onBottom && onLeft) return HTBOTTOMLEFT;
    if (onBottom && onRight) return HTBOTTOMRIGHT;
    if (onLeft) return HTLEFT;
    if (onRight) return HTRIGHT;
    if (onTop) return HTTOP;
    if (onBottom) return HTBOTTOM;
  }
  if (position.y() < headerHeight) {
    const QWidget* child = childAt(position);
    if (child == windowControls->maximizeButton()) {
      return HTMAXBUTTON;
    }
    if (child == handle) {
      return HTCLIENT;
    }
    for (const QWidget* widget = child; widget != nullptr; widget = widget->parentWidget()) {
      if (qobject_cast<const QAbstractButton*>(widget) || qobject_cast<const QLineEdit*>(widget) || qobject_cast<const QAbstractScrollArea*>(widget)) {
        return HTCLIENT;
      }
    }
    return HTCAPTION;
  }
  return HTCLIENT;
}

bool MainWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
  if (eventType != "windows_generic_MSG") {
    return QWidget::nativeEvent(eventType, message, result);
  }
  MSG* msg = static_cast<MSG*>(message);
  switch (msg->message) {
  case WM_NCCALCSIZE:
    if (msg->wParam) {
      if (IsZoomed(msg->hwnd)) {
        auto* parameters = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
        const UINT dpi = GetDpiForWindow(msg->hwnd);
        const int frameX = GetSystemMetricsForDpi(SM_CXFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        const int frameY = GetSystemMetricsForDpi(SM_CYFRAME, dpi) + GetSystemMetricsForDpi(SM_CXPADDEDBORDER, dpi);
        parameters->rgrc[0].left += frameX;
        parameters->rgrc[0].right -= frameX;
        parameters->rgrc[0].top += frameY;
        parameters->rgrc[0].bottom -= frameY;
      }
      *result = 0;
      return true;
    }
    break;
  case WM_NCHITTEST: {
    POINT point{GET_X_LPARAM(msg->lParam), GET_Y_LPARAM(msg->lParam)};
    ScreenToClient(msg->hwnd, &point);
    const qreal ratio = devicePixelRatioF();
    *result = hitTest(QPoint(qRound(point.x / ratio), qRound(point.y / ratio)));
    return true;
  }
  case WM_NCMOUSEMOVE:
    if (msg->wParam == HTMAXBUTTON) {
      windowControls->maximizeButton()->setNonClientState(true, isMaximizePressed);
      TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, msg->hwnd, 0};
      TrackMouseEvent(&track);
    } else {
      windowControls->maximizeButton()->setNonClientState(false, false);
    }
    break;
  case WM_NCMOUSELEAVE:
    isMaximizePressed = false;
    windowControls->maximizeButton()->setNonClientState(false, false);
    break;
  case WM_NCLBUTTONDOWN:
    if (msg->wParam == HTMAXBUTTON) {
      isMaximizePressed = true;
      windowControls->maximizeButton()->setNonClientState(true, true);
      *result = 0;
      return true;
    }
    break;
  case WM_NCLBUTTONUP:
    if (msg->wParam == HTMAXBUTTON) {
      isMaximizePressed = false;
      windowControls->maximizeButton()->setNonClientState(true, false);
      if (isMaximized()) {
        showNormal();
      } else {
        showMaximized();
      }
      *result = 0;
      return true;
    }
    break;
  }
  return QWidget::nativeEvent(eventType, message, result);
}

void MainWindow::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  const MARGINS margins{1, 1, 1, 1};
  DwmExtendFrameIntoClientArea(reinterpret_cast<HWND>(winId()), &margins);
  SetWindowPos(reinterpret_cast<HWND>(winId()), nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  if (!isSizeApplied) {
    isSizeApplied = true;
    resize(desiredSize);
  }
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  placeWindowControls();
}

void MainWindow::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::WindowStateChange) {
    windowControls->setMaximized(isMaximized());
  }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (!isMaximized() && !isMinimized()) {
    settings.setValue(QStringLiteral("window/size"), size());
  }
  QWidget::closeEvent(event);
}
