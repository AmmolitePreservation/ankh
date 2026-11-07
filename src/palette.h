#pragma once

#include <QColor>
#include <QString>

struct Palette {
  QColor base;
  QColor raised;
  QColor text;
  QColor secondary;
  QColor accent;
  QColor border;
  QColor success;
};

const Palette& ankhPalette();
QString buildStyleSheet(const Palette& colors);
