#include "appVersion.h"
#include "cleanStart.h"
#include "fatal.h"
#include "mainWindow.h"
#include "pointerCursors.h"
#include "updateInstaller.h"

#include <QApplication>
#include <QDateTime>
#include <QSettings>

int main(int argc, char* argv[])
{
  QApplication application(argc, argv);
  application.setApplicationName(QStringLiteral("Ankh"));
  application.setOrganizationName(QStringLiteral("AmmolitePreservation"));
  application.setApplicationVersion(QString::fromLatin1(appVersion));
  if (prefersSystem32Images()) {
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, QStringLiteral("AmmolitePreservation"), QStringLiteral("Ankh"));
    const QString restartKey = QStringLiteral("startup/restartedThroughExplorer");
    const QDateTime lastRestart = QDateTime::fromString(settings.value(restartKey).toString(), Qt::ISODate);
    if (lastRestart.isValid() && lastRestart.secsTo(QDateTime::currentDateTimeUtc()) < 60) {
      fatal(QStringLiteral("Windows starts Ankh with its 'Prefer system images' protection on, which stops every game's "
        "plugins from loading, and starting Ankh again through Windows Explorer did not turn it off. Look for it under "
        "Windows Security, App & browser control, Exploit protection, for the system or for ankh.exe."));
    }
    settings.setValue(restartKey, QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    settings.sync();
    restartThroughExplorer();
    return 0;
  }

  PointerCursors pointerCursors;
  application.installEventFilter(&pointerCursors);
  removeUpdateLeftovers();
  MainWindow window;
  window.show();
  return application.exec();
}
