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
};

const Palette& ankhPalette();
QString buildStyleSheet(const Palette& colors);