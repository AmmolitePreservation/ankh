#pragma once

#include "ammoliteClient.h"

#include <QColor>
#include <QRect>
#include <QString>
#include <QUrl>
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
  // An attached game that Steam starts has this, and no worlds or logins of its own.
  int steamAppID = 0;

  bool startsThroughSteam() const { return steamAppID > 0; }
};

QString initialsFor(const QString& title);
Game gameFromWorld(const World& world);
Game gameFromAttached(const AttachedGame& attached);
// What asks Steam to start the game, as Steam's own shortcuts do.
QUrl steamStartUrl(const Game& game);
QList<Package> packagesFor(const Game& game, const QList<Package>& packages);
QString roleLabel(const QString& role);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
