#pragma once

#include "ammoliteClient.h"

#include <QColor>
#include <QRect>
#include <QString>
#include <optional>

class QPainter;

struct Game {
  QString id;
  int serverID;
  QString title;
  QString edition;
  QString worldName;
  QString initials;
  QColor tint;
  QString status;
  std::optional<int> accountDomainID;
  QString accountDomain;
  bool isInstalled;
};

QString initialsFor(const QString& title);
Game gameFromWorld(const World& world);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
