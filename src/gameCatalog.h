#pragma once

#include <QColor>
#include <QList>
#include <QRect>
#include <QString>
#include <QStringList>
#include <optional>

class QPainter;

struct Character {
  QString name;
  QString summary;
};

struct Game {
  QString id;
  QString title;
  QString edition;
  QString initials;
  QColor tint;
  QString build;
  bool isInstalled;
  QStringList realms;
  QStringList accounts;
  std::optional<Character> recentCharacter;
};

const QList<Game>& gameCatalog();
const Game& gameWithId(const QString& id);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
