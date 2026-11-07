#pragma once

#include "skin.h"

#include <QWidget>

class GameDelegate;
class GameFilterProxy;
class QAction;
class QActionGroup;
class QLabel;
class QLineEdit;
class QListView;
class QMenu;
class QPushButton;
class QSettings;
class QStandardItemModel;

class LibraryPane : public QWidget {
  Q_OBJECT

public:
  enum class SortMode { custom, name, installState };

  explicit LibraryPane(QSettings& settings, QWidget* parent = nullptr);

  void applySkin(const Skin& skin);
  void selectGame(const QString& id);

signals:
  void gameSelected(const QString& id);

private:
  void applyFilters();
  void setSortMode(SortMode mode);
  void saveFavorites();

  QSettings& settings;
  QStandardItemModel* model;
  GameFilterProxy* proxy;
  GameDelegate* delegate;
  QLabel* countLabel;
  QLineEdit* search;
  QAction* searchAction;
  QAction* filterMenuAction;
  QMenu* filterMenu;
  QAction* installedOnlyAction;
  QActionGroup* sortGroup;
  QAction* customSortAction;
  QAction* nameSortAction;
  QAction* installSortAction;
  QPushButton* allChip;
  QPushButton* favoritesChip;
  QPushButton* nameChip;
  QLabel* emptyLabel;
  QListView* list;
};
