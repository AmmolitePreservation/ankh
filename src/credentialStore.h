#pragma once

#include <QString>
#include <optional>

class CredentialStore {
public:
  explicit CredentialStore(const QString& target);

  void save(const QString& secret) const;
  std::optional<QString> load() const;
  void remove() const;

private:
  QString target;
};
