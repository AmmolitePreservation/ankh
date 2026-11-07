#include "fatal.h"

#include <QMessageBox>
#include <cstdlib>

void fatal(const QString& message)
{
  QMessageBox::critical(nullptr, QStringLiteral("Ankh"), message);
  std::abort();
}
