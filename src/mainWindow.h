#pragma once

#include "skin.h"

#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QWidget>

class GamePage;
class LibraryPane;
class QLabel;
class QPushButton;
class QStackedWidget;
class QSvgWidget;
class SettingsPage;
class SettingsPane;
class TitleBar;
class UpdateChecker;

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
  void changeEvent(QEvent* event) override;
  void closeEvent(QCloseEvent* event) override;

private:
  void applySkin(const QString& name);
  void showLibrary();
  void showSettings();
  void setSidebarWidth(int width);
  void openUpdateSheet();
  long hitTest(const QPoint& position) const;

  QSettings settings;
  const Skin* skin;
  TitleBar* titleBar;
  QWidget* sidebar;
  SplitHandle* handle;
  QSvgWidget* wordmarkImage;
  QLabel* wordmarkText;
  QStackedWidget* leftStack;
  QStackedWidget* mainStack;
  LibraryPane* libraryPane;
  SettingsPane* settingsPane;
  GamePage* gamePage;
  SettingsPage* settingsPage;
  QPushButton* libraryNavigation;
  QPushButton* settingsButton;
  UpdateChecker* updateChecker;
  QTimer updateTimer;
  QString pendingVersion;
  QString pendingNotes;
  QUrl pendingPage;
  bool isMaximizePressed = false;
};
