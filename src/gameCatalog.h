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
  QString variant;
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
QList<Package> packagesFor(const Game& game, const QList<Package>& packages);
QString roleLabel(const QString& role);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
