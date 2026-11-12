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
  // An attached game: no realm, the player's own copy.
  bool isAttached = false;
  // An attached game is recognized by this file inside its folder. A museum game's is the file name of its program in
  // the installed client, empty where Ankh cannot start the game.
  QString executable;
  QString launcherConfig;
  // An attached game that Steam starts has this, and no worlds or logins of its own.
  int steamAppID = 0;
  // A museum game whose client signs the player in with their own account: Ankh hands it the means at Play and shows
  // no accounts.
  bool givesCredential = false;

  bool startsThroughSteam() const { return steamAppID > 0; }
};

QString initialsFor(const QString& title);
Game gameFromWorld(const World& world);
Game gameFromAttached(const AttachedGame& attached);
// What asks Steam to start the game, as Steam's own shortcuts do.
QUrl steamStartUrl(const Game& game);
// Where the game's client asks the site for the player's account in that world.
QUrl credentialUrl(const QUrl& siteUrl, const Game& game);
QList<Package> packagesFor(const Game& game, const QList<Package>& packages);
QString roleLabel(const QString& role);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
