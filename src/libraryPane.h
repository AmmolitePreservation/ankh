#pragma once

#include "gameCatalog.h"
#include "palette.h"

#include <QHash>
#include <QWidget>

class GameDelegate;
class GameFilterProxy;
class QAction;
class QLabel;
class QLineEdit;
class QListView;
class QPushButton;
class QSettings;
class QStandardItemModel;

class LibraryPane : public QWidget {
  Q_OBJECT

public:
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
  void updateToggleIcons();
  void saveFavorites();

  QSettings& settings;
  Palette colors;
  QStandardItemModel* model;
  GameFilterProxy* proxy;
  GameDelegate* delegate;
  QLineEdit* search;
  QAction* searchAction;
  QPushButton* favoritesButton;
  QPushButton* installedButton;
  QWidget* messageBox;
  QLabel* emptyLabel;
  QPushButton* actionButton;
  QListView* list;
  QHash<QString, Game> games;
  QString message;
};
