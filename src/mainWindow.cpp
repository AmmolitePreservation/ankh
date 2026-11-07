#include "mainWindow.h"

#include "appVersion.h"
#include "captionButton.h"
#include "edgeGrain.h"
#include "fatal.h"
#include "gameCatalog.h"
#include "gamePage.h"
#include "icons.h"
#include "libraryPane.h"
#include "settingsPage.h"
#include "settingsPane.h"
#include "updateChecker.h"
#include "updateSheet.h"
#include "windowControls.h"

#include <QAbstractScrollArea>
#include <QFrame>
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
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
constexpr int headerHeight = 84;
constexpr int resizeBorder = 6;

}

class UserMenuButton : public QPushButton {
public:
  UserMenuButton(const QString& name, QWidget* parent)
    : QPushButton(parent), userName(name)
  {
    setCursor(Qt::PointingHandCursor);
    setFlat(true);
    setAccessibleName(QStringLiteral("%1 menu").arg(name));
    setFixedHeight(48);
    QFont measured = font();
    measured.setPixelSize(18);
    setFixedWidth(54 + QFontMetrics(measured).horizontalAdvance(name) + 46);
  }

  void setColors(const QColor& text, const QColor& secondary, const QColor& border)
  {
    textColor = text;
    secondaryColor = secondary;
    borderColor = border;
    update();
  }

protected:
  void paintEvent(QPaintEvent*) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal ratio = devicePixelRatioF();
    painter.setPen(QPen(QColor("#4A4E50"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(QRectF(0.5, 4.5, 39, 39));
    painter.drawPixmap(QRect(9, 13, 22, 22), iconPixmap(IconKind::user, 22, textColor, ratio));
    painter.setPen(textColor);
    QFont textFont = font();
    textFont.setPixelSize(18);
    painter.setFont(textFont);
    painter.drawText(QRect(54, 0, width() - 54 - 34, height()), Qt::AlignVCenter | Qt::AlignLeft, userName);
    painter.drawPixmap(QRect(width() - 26, 15, 18, 18), iconPixmap(IconKind::chevronDown, 18, textColor, ratio));
    if (hasFocus()) {
      painter.setPen(QPen(borderColor, 1, Qt::DotLine));
      painter.drawRect(rect().adjusted(0, 0, -1, -1));
    }
  }

private:
  QString userName;
  QColor textColor;
  QColor secondaryColor;
  QColor borderColor;
};
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
    settingsPage(new SettingsPage(this)),
    libraryNavigation(new QPushButton(QStringLiteral("Library"), this)),
    settingsButton(new QPushButton(QStringLiteral("Settings"), this)),
    activityButton(new QPushButton(QStringLiteral("Activity"), this)),
    userMenuButton(nullptr),
    updateChecker(new UpdateChecker(QString::fromLatin1(appVersion), this))
{
  const QString userName = qEnvironmentVariable("USERNAME");
  if (userName.isEmpty()) {
    fatal(QStringLiteral("The USERNAME environment variable is not set."));
  }
  userMenuButton = new UserMenuButton(userName, this);

  setWindowTitle(QStringLiteral("Ankh"));
  setWindowIcon(QIcon(QStringLiteral(":/ankh.ico")));
  setMinimumSize(1180, 760);
  resize(settings.value(QStringLiteral("window/size"), QSize(1600, 1000)).toSize());
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
  settingsButton->setCursor(Qt::PointingHandCursor);
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
  mainHeaderLayout->setContentsMargins(22, 10, 36, 6);
  mainHeaderLayout->setSpacing(8);
  mainHeaderLayout->addWidget(libraryNavigation);
  mainHeaderLayout->addStretch(1);
  mainHeaderLayout->addWidget(userMenuButton, 0, Qt::AlignVCenter);

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
  connect(userMenu->addAction(QStringLiteral("Settings")), &QAction::triggered, this, [this] { showSettings(0); });
  connect(userMenu->addAction(QStringLiteral("Attributions")), &QAction::triggered, this, [this] { showSettings(1); });
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
    windowControls->updateButton()->setVisible(false);
    settingsPage->setUpdateStatus(QStringLiteral("Ankh is up to date."));
  });
  connect(updateChecker, &UpdateChecker::noReleases, this, [this] {
    windowControls->updateButton()->setVisible(false);
    settingsPage->setUpdateStatus(QStringLiteral("No releases have been published yet."));
  });
  connect(updateChecker, &UpdateChecker::failed, this, [this](const QString& message) {
    settingsPage->setUpdateStatus(QStringLiteral("Could not check for updates: %1").arg(message));
  });
  connect(updateChecker, &UpdateChecker::updateFound, this, [this](const QString& version, const QString& notes, const QUrl& page) {
    pendingVersion = version;
    pendingNotes = notes;
    pendingPage = page;
    windowControls->updateButton()->setToolTip(QStringLiteral("Update available: %1").arg(version));
    windowControls->updateButton()->setVisible(true);
    settingsPage->setUpdateStatus(QStringLiteral("Update available: %1").arg(version));
  });
  connect(windowControls->updateButton(), &QAbstractButton::clicked, this, &MainWindow::openUpdateSheet);

  updateTimer.setInterval(24 * 60 * 60 * 1000);
  connect(&updateTimer, &QTimer::timeout, updateChecker, &UpdateChecker::check);
  updateTimer.start();
  QTimer::singleShot(0, updateChecker, &UpdateChecker::check);

  const QString selected = settings.value(QStringLiteral("library/selected")).toString();
  if (!selected.isEmpty()) {
    libraryPane->selectGame(selected);
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
  mainStack->setCurrentIndex(settings.contains(QStringLiteral("library/selected")) ? 1 : 0);
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
