#include "installInfo.h"

#include <QCoreApplication>
#include <QDir>

namespace {

bool isUnder(const QString& folder, const QString& root)
{
  return !root.isEmpty() && folder.startsWith(QDir::toNativeSeparators(root), Qt::CaseInsensitive);
}

}

QString installFolder()
{
  return QDir::toNativeSeparators(QCoreApplication::applicationDirPath());
}

InstallScope installScope()
{
  const QString folder = installFolder();
  if (isUnder(folder, qEnvironmentVariable("ProgramFiles")) || isUnder(folder, qEnvironmentVariable("ProgramFiles(x86)"))) {
    return InstallScope::allUsers;
  }
  if (isUnder(folder, qEnvironmentVariable("LOCALAPPDATA"))) {
    return InstallScope::thisUser;
  }
  return InstallScope::buildFolder;
}
