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
  star,
  starFilled,
  checkBadge,
  statusDot,
  play,
  windowMinimize,
  windowMaximize,
  windowRestore,
  windowClose,
  download,
};

QPixmap iconPixmap(IconKind kind, int size, const QColor& color, qreal ratio);
QIcon makeIcon(IconKind kind, int size, const QColor& color);
