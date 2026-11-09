#pragma once

#include <QString>
#include <QStringList>
#include <optional>

// Logins for games Ammolite does not host (attached games), kept in Windows Credential Manager for this Windows
// user, one entry per game edition and account name. The password never leaves this computer.
class GameLoginStore {
public:
  GameLoginStore(const QString& game, const QString& variant);

  QStringList usernames() const;
  void save(const QString& username, const QString& password) const;
  std::optional<QString> password(const QString& username) const;
  void remove(const QString& username) const;

private:
  QString targetFor(const QString& username) const;

  QString prefix;
};
