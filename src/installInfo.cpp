#include "installInfo.h"

#include "fatal.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <string>

#define NOMINMAX
#include <windows.h>

namespace {

// Inno Setup records each install, with the folder the user chose, under the AppId of installer/ankh.iss.
const wchar_t* const uninstallKey =
  L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{6E1F4C3A-5B7D-4E0A-9C21-A47D2B90E6F1}_is1";
const wchar_t* const appPathValue = L"Inno Setup: App Path";

QString recordedFolder(HKEY root, const QString& rootName)
{
  DWORD byteSize = 0;
  LSTATUS status = RegGetValueW(root, uninstallKey, appPathValue, RRF_RT_REG_SZ, nullptr, nullptr, &byteSize);
  if (status == ERROR_FILE_NOT_FOUND) {
    return QString();
  }
  std::wstring value(byteSize / sizeof(wchar_t), L'\0');
  if (status == ERROR_SUCCESS) {
    status = RegGetValueW(root, uninstallKey, appPathValue, RRF_RT_REG_SZ, nullptr, value.data(), &byteSize);
  }
  if (status != ERROR_SUCCESS) {
    fatal(QStringLiteral("Could not read Ankh's install record in %1 (error %2).").arg(rootName).arg(status));
  }
  return QString::fromWCharArray(value.c_str());
}

bool isThisFolder(const QString& recorded)
{
  const QString canonical = QFileInfo(recorded).canonicalFilePath();
  return !canonical.isEmpty()
    && canonical.compare(QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath(), Qt::CaseInsensitive) == 0;
}

}

QString installFolder()
{
  return QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
}

InstallScope installScope()
{
  if (isThisFolder(recordedFolder(HKEY_LOCAL_MACHINE, QStringLiteral("HKEY_LOCAL_MACHINE")))) {
    return InstallScope::allUsers;
  }
  if (isThisFolder(recordedFolder(HKEY_CURRENT_USER, QStringLiteral("HKEY_CURRENT_USER")))) {
    return InstallScope::thisUser;
  }
  return InstallScope::notInstalled;
}
