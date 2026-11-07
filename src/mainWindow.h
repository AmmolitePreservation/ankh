#pragma once

#include "palette.h"
#include "updateChecker.h"

#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QWidget>

class EdgeGrain;
class GamePage;
class LibraryPane;
class QPushButton;
class QStackedWidget;
class QSvgWidget;
class SettingsPage;
class SettingsPane;
class UpdateChecker;
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
  void setSidebarWidth(int width);
  void placeWindowControls();
  void openUpdateSheet();
  long hitTest(const QPoint& position) const;

  QSettings settings;
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
  UpdateChecker* updateChecker;
  QTimer updateTimer;
  UpdateOffer pendingOffer;
  bool isMaximizePressed = false;
};
