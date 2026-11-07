#pragma once

#include "skin.h"

#include <QWidget>

class GameDelegate;
class GameFilterProxy;
class QComboBox;
class QLabel;
class QLineEdit;
class QListView;
class QSettings;
class QStandardItemModel;

class LibraryPane : public QWidget {
  Q_OBJECT

public:
  explicit LibraryPane(QSettings& settings, QWidget* parent = nullptr);

  void applySkin(const Skin& skin);
  void selectGame(const QString& id);

signals:
  void gameSelected(const QString& id);

private:
  void applyFilters();
  void saveFavorites();

  QSettings& settings;
  QStandardItemModel* model;
  GameFilterProxy* proxy;
  GameDelegate* delegate;
  QLineEdit* search;
  QComboBox* filterChoice;
  QComboBox* sortChoice;
  QLabel* countLabel;
  QLabel* emptyLabel;
  QListView* list;
};
