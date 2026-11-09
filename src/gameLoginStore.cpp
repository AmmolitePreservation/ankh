#include "gameLoginStore.h"

#include "credentialStore.h"
#include "fatal.h"

#include <algorithm>

#define NOMINMAX
#include <windows.h>
#include <wincred.h>

namespace {

const QString root = QStringLiteral("Ankh/AmmolitePreservation/gameLogin/");

// Credential Manager target names are one string, so the parts are escaped to keep a slash in a game's name from
// reading as a separator.
QString escaped(const QString& part)
{
  QString result = part;
  result.replace(QLatin1Char('%'), QStringLiteral("%25"));
  result.replace(QLatin1Char('/'), QStringLiteral("%2F"));
  result.replace(QLatin1Char('*'), QStringLiteral("%2A"));
  return result;
}

}

GameLoginStore::GameLoginStore(const QString& game, const QString& variant)
  : prefix(root + escaped(game) + QLatin1Char('/') + escaped(variant) + QLatin1Char('/'))
{
}

QString GameLoginStore::targetFor(const QString& username) const
{
  return prefix + escaped(username);
}

QStringList GameLoginStore::usernames() const
{
  DWORD count = 0;
  PCREDENTIALW* credentials = nullptr;
  const std::wstring filter = (prefix + QLatin1Char('*')).toStdWString();
  if (!CredEnumerateW(filter.c_str(), 0, &count, &credentials)) {
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND) {
      return {};
    }
    fatal(QStringLiteral("Windows Credential Manager could not list the game logins (error %1).").arg(code));
  }
  QStringList names;
  for (DWORD index = 0; index < count; ++index) {
    if (credentials[index]->UserName != nullptr) {
      names.append(QString::fromWCharArray(credentials[index]->UserName));
    }
  }
  CredFree(credentials);
  std::sort(names.begin(), names.end(), [](const QString& left, const QString& right) {
    return left.compare(right, Qt::CaseInsensitive) < 0;
  });
  return names;
}

void GameLoginStore::save(const QString& username, const QString& password) const
{
  if (username.trimmed().isEmpty() || username != username.trimmed()) {
    fatal(QStringLiteral("A game login needs a name without surrounding spaces."));
  }
  const QByteArray blob = password.toUtf8();
  const std::wstring targetName = targetFor(username).toStdWString();
  std::wstring userName = username.toStdWString();
  CREDENTIALW credential = {};
  credential.Type = CRED_TYPE_GENERIC;
  credential.TargetName = const_cast<LPWSTR>(targetName.c_str());
  credential.UserName = userName.data();
  credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
  credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(blob.constData()));
  credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
  if (!CredWriteW(&credential, 0)) {
    fatal(QStringLiteral("Windows Credential Manager could not save the game login (error %1).").arg(GetLastError()));
  }
}

std::optional<QString> GameLoginStore::password(const QString& username) const
{
  return CredentialStore(targetFor(username)).load();
}

void GameLoginStore::remove(const QString& username) const
{
  CredentialStore(targetFor(username)).remove();
}
