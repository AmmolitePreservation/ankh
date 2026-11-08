#include "credentialStore.h"

#include "fatal.h"

#include <string>

#define NOMINMAX
#include <windows.h>
#include <wincred.h>

namespace {

[[noreturn]] void failWith(const QString& action, DWORD code)
{
  fatal(QStringLiteral("Windows Credential Manager could not %1 (error %2).").arg(action).arg(code));
}

}

CredentialStore::CredentialStore(const QString& target)
  : target(target)
{
}

void CredentialStore::save(const QString& secret) const
{
  const QByteArray blob = secret.toUtf8();
  const std::wstring targetName = target.toStdWString();
  std::wstring userName = L"ankh";
  CREDENTIALW credential = {};
  credential.Type = CRED_TYPE_GENERIC;
  credential.TargetName = const_cast<LPWSTR>(targetName.c_str());
  credential.UserName = userName.data();
  credential.CredentialBlobSize = static_cast<DWORD>(blob.size());
  credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(blob.constData()));
  credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
  if (!CredWriteW(&credential, 0)) {
    failWith(QStringLiteral("save the sign-in"), GetLastError());
  }
}

std::optional<QString> CredentialStore::load() const
{
  PCREDENTIALW credential = nullptr;
  if (!CredReadW(target.toStdWString().c_str(), CRED_TYPE_GENERIC, 0, &credential)) {
    const DWORD code = GetLastError();
    if (code == ERROR_NOT_FOUND) {
      return std::nullopt;
    }
    failWith(QStringLiteral("read the sign-in"), code);
  }
  const QString secret = QString::fromUtf8(reinterpret_cast<const char*>(credential->CredentialBlob), static_cast<qsizetype>(credential->CredentialBlobSize));
  CredFree(credential);
  return secret;
}

void CredentialStore::remove() const
{
  if (!CredDeleteW(target.toStdWString().c_str(), CRED_TYPE_GENERIC, 0)) {
    const DWORD code = GetLastError();
    if (code != ERROR_NOT_FOUND) {
      failWith(QStringLiteral("remove the sign-in"), code);
    }
  }
}
