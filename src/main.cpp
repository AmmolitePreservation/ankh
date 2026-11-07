#include "appVersion.h"
#include "mainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
  QApplication application(argc, argv);
  application.setApplicationName(QStringLiteral("Ankh"));
  application.setOrganizationName(QStringLiteral("AmmolitePreservation"));
  application.setApplicationVersion(QString::fromLatin1(appVersion));

  MainWindow window;
  window.show();
  return application.exec();
}
