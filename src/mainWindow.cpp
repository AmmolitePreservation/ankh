#include "mainWindow.h"

#include "appVersion.h"
#include "captionButton.h"
#include "gameCatalog.h"
#include "gamePage.h"
#include "libraryPane.h"
#include "settingsPage.h"
#include "settingsPane.h"
#include "titleBar.h"
#include "updateChecker.h"
#include "updateSheet.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QStackedWidget>
#include <QSvgWidget>
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
constexpr int headerHeight = 64;
constexpr int resizeBorder = 6;

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
    skin(&skinNamed(settings.value(QStringLiteral("appearance/skin"), QStringLiteral("Ankh")).toString())),
    titleBar(new TitleBar(this)),
    sidebar(new QWidget(this)),
    handle(new SplitHandle(this)),
    wordmarkImage(new QSvgWidget(QStringLiteral(":/ankhWordmark.svg"), this)),
    wordmarkText(new QLabel(QStringLiteral("MMOnomicon"), this)),
    leftStack(new QStackedWidget(this)),
    mainStack(new QStackedWidget(this)),
    libraryPane(new LibraryPane(settings, this)),
    settingsPane(new SettingsPane(this)),
    gamePage(new GamePage(this)),
    settingsPage(new SettingsPage(skin->name, this)),
    libraryNavigation(new QPushButton(QStringLiteral("Library"), this)),
    settingsButton(new QPushButton(QStringLiteral("Settings"), this)),
    updateChecker(new UpdateChecker(QString::fromLatin1(appVersion), this))
{
  setWindowTitle(QStringLiteral("Ankh"));
  setWindowIcon(QIcon(QStringLiteral(":/ankh.ico")));
  setMinimumSize(1180, 760);
  resize(settings.value(QStringLiteral("window/size"), QSize(1280, 800)).toSize());
  winId();

  sidebar->setObjectName(QStringLiteral("sidebar"));
  auto* sidebarHeader = new QWidget(sidebar);
  sidebarHeader->setObjectName(QStringLiteral("sidebarHeader"));
  sidebarHeader->setFixedHeight(headerHeight);
  wordmarkImage->setFixedSize(140, 36);
  wordmarkImage->setAccessibleName(QStringLiteral("Ankh"));
  QFont wordmarkFont(QStringLiteral("Georgia"), 17);
  wordmarkText->setFont(wordmarkFont);
  wordmarkText->setAccessibleName(QStringLiteral("MMOnomicon"));
  auto* headerLayout = new QHBoxLayout(sidebarHeader);
  headerLayout->setContentsMargins(20, 0, 20, 0);
  headerLayout->addWidget(wordmarkImage);
  headerLayout->addWidget(wordmarkText);
  headerLayout->addStretch(1);

  settingsButton->setProperty("link", true);
  settingsButton->setCursor(Qt::PointingHandCursor);
  leftStack->addWidget(libraryPane);
  leftStack->addWidget(settingsPane);
  auto* foot = new QWidget(sidebar);
  foot->setObjectName(QStringLiteral("sidebar"));
  auto* footLayout = new QHBoxLayout(foot);
  footLayout->setContentsMargins(16, 6, 16, 10);
  footLayout->addWidget(settingsButton);
  footLayout->addStretch(1);

  auto* sidebarLayout = new QVBoxLayout(sidebar);
  sidebarLayout->setContentsMargins(0, 0, 0, 0);
  sidebarLayout->setSpacing(0);
  sidebarLayout->addWidget(sidebarHeader);
  sidebarLayout->addWidget(leftStack, 1);
  sidebarLayout->addWidget(foot);

  libraryNavigation->setProperty("navigation", true);
  libraryNavigation->setCheckable(true);
  libraryNavigation->setChecked(true);
  auto* profile = new QLabel(QStringLiteral("Household"), this);
  profile->setProperty("secondary", true);
  auto* mainHeader = new QWidget(this);
  mainHeader->setFixedHeight(headerHeight);
  auto* mainHeaderLayout = new QHBoxLayout(mainHeader);
  mainHeaderLayout->setContentsMargins(28, 0, 28, 0);
  mainHeaderLayout->addWidget(libraryNavigation);
  mainHeaderLayout->addStretch(1);
  mainHeaderLayout->addWidget(profile);

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

  auto* body = new QHBoxLayout;
  body->setContentsMargins(0, 0, 0, 0);
  body->setSpacing(0);
  body->addWidget(sidebar);
  body->addWidget(handle);
  body->addLayout(mainColumn, 1);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(titleBar);
  layout->addLayout(body, 1);

  setSidebarWidth(settings.value(QStringLiteral("window/sidebarWidth"), defaultSidebarWidth).toInt());
  applySkin(skin->name);

  connect(titleBar, &TitleBar::minimizeRequested, this, &QWidget::showMinimized);
  connect(titleBar, &TitleBar::maximizeRequested, this, [this] {
    if (isMaximized()) {
      showNormal();
    } else {
      showMaximized();
    }
  });
  connect(titleBar, &TitleBar::closeRequested, this, &QWidget::close);
  connect(handle, &SplitHandle::dragged, this, [this](int globalX) {
    setSidebarWidth(globalX - mapToGlobal(QPoint(0, 0)).x());
  });
  connect(libraryNavigation, &QPushButton::clicked, this, &MainWindow::showLibrary);
  connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
  connect(settingsPane, &SettingsPane::sectionSelected, settingsPage, &SettingsPage::showSection);
  connect(settingsPage, &SettingsPage::skinChosen, this, &MainWindow::applySkin);
  connect(settingsPage, &SettingsPage::updateCheckRequested, updateChecker, &UpdateChecker::check);
  connect(libraryPane, &LibraryPane::gameSelected, this, [this](const QString& id) {
    gamePage->showGame(gameWithId(id));
    settings.setValue(QStringLiteral("library/selected"), id);
    showLibrary();
  });

  connect(updateChecker, &UpdateChecker::checking, this, [this] {
    settingsPage->setUpdateStatus(QStringLiteral("Checking for updates..."));
  });
  connect(updateChecker, &UpdateChecker::upToDate, this, [this] {
    titleBar->updateButton()->setVisible(false);
    settingsPage->setUpdateStatus(QStringLiteral("Ankh is up to date."));
  });
  connect(updateChecker, &UpdateChecker::noReleases, this, [this] {
    titleBar->updateButton()->setVisible(false);
    settingsPage->setUpdateStatus(QStringLiteral("No releases have been published yet."));
  });
  connect(updateChecker, &UpdateChecker::failed, this, [this](const QString& message) {
    settingsPage->setUpdateStatus(QStringLiteral("Could not check for updates: %1").arg(message));
  });
  connect(updateChecker, &UpdateChecker::updateFound, this, [this](const QString& version, const QString& notes, const QUrl& page) {
    pendingVersion = version;
    pendingNotes = notes;
    pendingPage = page;
    titleBar->updateButton()->setToolTip(QStringLiteral("Update available: %1").arg(version));
    titleBar->updateButton()->setVisible(true);
    settingsPage->setUpdateStatus(QStringLiteral("Update available: %1").arg(version));
  });
  connect(titleBar->updateButton(), &QAbstractButton::clicked, this, &MainWindow::openUpdateSheet);

  updateTimer.setInterval(24 * 60 * 60 * 1000);
  connect(&updateTimer, &QTimer::timeout, updateChecker, &UpdateChecker::check);
  updateTimer.start();
  QTimer::singleShot(0, updateChecker, &UpdateChecker::check);

  const QString selected = settings.value(QStringLiteral("library/selected")).toString();
  if (!selected.isEmpty()) {
    libraryPane->selectGame(selected);
  }
}

void MainWindow::applySkin(const QString& name)
{
  skin = &skinNamed(name);
  settings.setValue(QStringLiteral("appearance/skin"), name);
  QPalette applicationPalette = qApp->palette();
  applicationPalette.setColor(QPalette::Link, skin->accent);
  applicationPalette.setColor(QPalette::LinkVisited, skin->accent);
  qApp->setPalette(applicationPalette);
  qApp->setStyleSheet(buildStyleSheet(*skin));
  wordmarkImage->setVisible(skin->usesWordmarkImage);
  wordmarkText->setVisible(!skin->usesWordmarkImage);
  handle->setLineColor(skin->border);
  titleBar->applySkin(*skin);
  libraryPane->applySkin(*skin);
  QPalette palette = handle->palette();
  palette.setColor(QPalette::Window, skin->base);
  handle->setPalette(palette);
}

void MainWindow::showLibrary()
{
  libraryNavigation->setChecked(true);
  leftStack->setCurrentWidget(libraryPane);
  mainStack->setCurrentIndex(settings.contains(QStringLiteral("library/selected")) ? 1 : 0);
}

void MainWindow::showSettings()
{
  libraryNavigation->setChecked(false);
  leftStack->setCurrentWidget(settingsPane);
  mainStack->setCurrentWidget(settingsPage);
}

void MainWindow::setSidebarWidth(int width)
{
  const int clamped = qBound(minimumSidebarWidth, width, maximumSidebarWidth);
  sidebar->setFixedWidth(clamped);
  settings.setValue(QStringLiteral("window/sidebarWidth"), clamped);
}

void MainWindow::openUpdateSheet()
{
  UpdateSheet sheet(pendingVersion, pendingNotes, pendingPage, this);
  sheet.exec();
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
  if (position.y() < TitleBar::barHeight) {
    const QPoint local = titleBar->mapFrom(this, position);
    if (titleBar->isMaximizeButtonAt(local)) {
      return HTMAXBUTTON;
    }
    if (titleBar->isCaptionAt(local)) {
      return HTCAPTION;
    }
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
      titleBar->maximizeButton()->setNonClientState(true, isMaximizePressed);
      TRACKMOUSEEVENT track{sizeof(TRACKMOUSEEVENT), TME_LEAVE | TME_NONCLIENT, msg->hwnd, 0};
      TrackMouseEvent(&track);
    } else {
      titleBar->maximizeButton()->setNonClientState(false, false);
    }
    break;
  case WM_NCMOUSELEAVE:
    isMaximizePressed = false;
    titleBar->maximizeButton()->setNonClientState(false, false);
    break;
  case WM_NCLBUTTONDOWN:
    if (msg->wParam == HTMAXBUTTON) {
      isMaximizePressed = true;
      titleBar->maximizeButton()->setNonClientState(true, true);
      *result = 0;
      return true;
    }
    break;
  case WM_NCLBUTTONUP:
    if (msg->wParam == HTMAXBUTTON) {
      isMaximizePressed = false;
      titleBar->maximizeButton()->setNonClientState(true, false);
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
}

void MainWindow::changeEvent(QEvent* event)
{
  QWidget::changeEvent(event);
  if (event->type() == QEvent::WindowStateChange) {
    titleBar->setMaximized(isMaximized());
  }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
  if (!isMaximized() && !isMinimized()) {
    settings.setValue(QStringLiteral("window/size"), size());
  }
  QWidget::closeEvent(event);
}
