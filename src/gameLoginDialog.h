#pragma once

#include "gameCatalog.h"
#include "gameLoginStore.h"

#include <QDialog>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

// The logins kept on this computer for an attached game: add one, replace its password, or remove it. Passwords
// are typed, saved to Windows Credential Manager, and never shown again.
class GameLoginDialog : public QDialog {
  Q_OBJECT

public:
  GameLoginDialog(const Game& game, QWidget* parent = nullptr);

protected:
  void showEvent(QShowEvent* event) override;

private:
  void refreshList();
  void save();
  void removeSelected();

  GameLoginStore store;
  QListWidget* list;
  QLineEdit* username;
  QLineEdit* password;
  QPushButton* saveButton;
  QPushButton* removeButton;
  QLabel* note;
};
