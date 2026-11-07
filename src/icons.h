#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>

enum class IconKind {
  search,
  chevronDown,
  chevronRight,
  settings,
  activity,
  user,
  check,
  dot,
  play,
};

QPixmap iconPixmap(IconKind kind, int size, const QColor& color, qreal ratio);
QIcon makeIcon(IconKind kind, int size, const QColor& color);
