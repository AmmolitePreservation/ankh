#include "cleanStart.h"

#include "fatal.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#define NOMINMAX
#include <windows.h>

bool prefersSystem32Images()
{
  PROCESS_MITIGATION_IMAGE_LOAD_POLICY policy = {};
  if (!GetProcessMitigationPolicy(GetCurrentProcess(), ProcessImageLoadPolicy, &policy, sizeof(policy))) {
    fatal(QStringLiteral("Could not read Ankh's image load policy (error %1).").arg(GetLastError()));
  }
  return policy.PreferSystem32Images != 0;
}

void letGameTakeForeground(qint64 processID)
{
  // Windows refuses this while Ankh is not the foreground window, and then the game should not take the front either.
  AllowSetForegroundWindow(static_cast<DWORD>(processID));
}

void restartThroughExplorer()
{
  wchar_t windowsFolder[MAX_PATH];
  const UINT length = GetWindowsDirectoryW(windowsFolder, MAX_PATH);
  if (length == 0 || length >= MAX_PATH) {
    fatal(QStringLiteral("Could not find the Windows folder (error %1).").arg(GetLastError()));
  }
  const QString explorer = QDir(QString::fromWCharArray(windowsFolder, static_cast<int>(length))).filePath(QStringLiteral("explorer.exe"));
  if (!QProcess::startDetached(explorer, {QDir::toNativeSeparators(QCoreApplication::applicationFilePath())})) {
    fatal(QStringLiteral("Could not ask Windows Explorer to start Ankh again."));
  }
}
