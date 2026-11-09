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
  // An attached game: no realm, the player's own copy, recognized by this file inside its folder.
  bool isAttached = false;
  QString executable;
  QString launcherConfig;
};

QString initialsFor(const QString& title);
Game gameFromWorld(const World& world);
Game gameFromAttached(const AttachedGame& attached);
QList<Package> packagesFor(const Game& game, const QList<Package>& packages);
QString roleLabel(const QString& role);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
