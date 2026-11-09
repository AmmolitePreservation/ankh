#include "gameLoginDialog.h"

#include "darkTitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QLabel* makeLabel(const QString& text, const char* role, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  if (role != nullptr) {
    label->setProperty(role, true);
  }
  label->setWordWrap(true);
  return label;
}

}

GameLoginDialog::GameLoginDialog(const Game& game, QWidget* parent)
  : QDialog(parent),
    store(game.title, game.variant),
    list(new QListWidget(this)),
    username(new QLineEdit(this)),
    password(new QLineEdit(this)),
    saveButton(new QPushButton(QStringLiteral("Save login"), this)),
    removeButton(new QPushButton(QStringLiteral("Remove"), this)),
    note(makeLabel(QString(), "secondary", this))
{
  setWindowTitle(QStringLiteral("%1 logins").arg(game.title));
  setMinimumWidth(560);
  list->setAccessibleName(QStringLiteral("Game logins"));
  username->setAccessibleName(QStringLiteral("Game username"));
  username->setPlaceholderText(QStringLiteral("Account name"));
  password->setAccessibleName(QStringLiteral("Game password"));
  password->setPlaceholderText(QStringLiteral("Password"));
  password->setEchoMode(QLineEdit::Password);
  saveButton->setObjectName(QStringLiteral("primary"));
  removeButton->setAccessibleName(QStringLiteral("Remove game login"));
  auto* closeButton = new QPushButton(QStringLiteral("Close"), this);

  auto* fields = new QHBoxLayout;
  fields->setSpacing(8);
  fields->addWidget(username, 1);
  fields->addWidget(password, 1);
  fields->addWidget(saveButton);
  auto* buttons = new QHBoxLayout;
  buttons->setSpacing(8);
  buttons->addWidget(removeButton);
  buttons->addStretch(1);
  buttons->addWidget(closeButton);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(12);
  layout->addWidget(makeLabel(QStringLiteral("%1 logins").arg(game.title), "heading", this));
  layout->addWidget(makeLabel(QStringLiteral("These logins stay on this computer, in Windows Credential Manager for your Windows "
    "account. Ankh uses one to sign in to the game's own servers when you press Play; the Ammolite site never sees them."), "secondary", this));
  layout->addWidget(list, 1);
  layout->addWidget(makeLabel(QStringLiteral("Add a login, or type an existing name to replace its password"), "fieldLabel", this));
  layout->addLayout(fields);
  layout->addWidget(note);
  layout->addLayout(buttons);

  connect(saveButton, &QPushButton::clicked, this, &GameLoginDialog::save);
  connect(password, &QLineEdit::returnPressed, this, &GameLoginDialog::save);
  connect(removeButton, &QPushButton::clicked, this, &GameLoginDialog::removeSelected);
  connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(list, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current) {
    removeButton->setEnabled(current != nullptr);
    if (current != nullptr) {
      username->setText(current->text());
    }
  });
  connect(username, &QLineEdit::textChanged, this, [this] { saveButton->setEnabled(!username->text().trimmed().isEmpty() && !password->text().isEmpty()); });
  connect(password, &QLineEdit::textChanged, this, [this] { saveButton->setEnabled(!username->text().trimmed().isEmpty() && !password->text().isEmpty()); });
  saveButton->setEnabled(false);
  removeButton->setEnabled(false);
  refreshList();
}

void GameLoginDialog::refreshList()
{
  list->clear();
  list->addItems(store.usernames());
  note->setText(list->count() == 0 ? QStringLiteral("No login saved yet.") : QString());
  note->setVisible(!note->text().isEmpty());
}

void GameLoginDialog::save()
{
  const QString name = username->text().trimmed();
  if (name.isEmpty() || password->text().isEmpty()) {
    return;
  }
  const bool isReplacing = store.usernames().contains(name);
  store.save(name, password->text());
  password->clear();
  refreshList();
  note->setText(isReplacing ? QStringLiteral("Password of %1 replaced.").arg(name) : QStringLiteral("%1 saved.").arg(name));
  note->setVisible(true);
}

void GameLoginDialog::removeSelected()
{
  QListWidgetItem* current = list->currentItem();
  if (current == nullptr) {
    return;
  }
  const QString name = current->text();
  store.remove(name);
  refreshList();
  note->setText(QStringLiteral("%1 removed.").arg(name));
  note->setVisible(true);
}

void GameLoginDialog::showEvent(QShowEvent* event)
{
  applyDarkTitleBar(this);
  QDialog::showEvent(event);
}
