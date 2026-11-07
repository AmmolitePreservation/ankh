#pragma once

#include "gameCatalog.h"

#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;
class QTabBar;

class GamePage : public QWidget {
  Q_OBJECT

public:
  explicit GamePage(QWidget* parent = nullptr);

  void showGame(const Game& game);

private:
  QLabel* iconLabel;
  QLabel* titleLabel;
  QLabel* editionLabel;
  QLabel* aboutLabel;
  QLabel* statusLabel;
  QPushButton* primaryButton;
  QTabBar* tabs;
  QStackedWidget* pages;
};
