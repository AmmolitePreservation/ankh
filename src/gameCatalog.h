#pragma once

#include <QColor>
#include <QList>
#include <QRect>
#include <QString>

class QPainter;

struct Game {
  QString id;
  QString title;
  QString edition;
  QString initials;
  QColor tint;
  bool isInstalled;
};

const QList<Game>& gameCatalog();
const Game& gameWithId(const QString& id);
void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint);
