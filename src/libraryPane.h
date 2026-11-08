#pragma once

#include "gameCatalog.h"
#include "palette.h"

#include <QHash>
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

  void applyPalette(const Palette& colors);
  void showGames(const QList<Game>& games);
  void showMessage(const QString& text, const QString& actionText);
  void selectGame(const QString& id);

signals:
  void gameSelected(const Game& game);
  void actionRequested();

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;

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
  QWidget* messageBox;
  QLabel* emptyLabel;
  QPushButton* actionButton;
  QListView* list;
  QHash<QString, Game> games;
  QString message;
};
