#include "gameCatalog.h"

#include <QFont>
#include <QPainter>

QString initialsFor(const QString& title)
{
  static const QStringList skipped = {QStringLiteral("of"), QStringLiteral("the"), QStringLiteral("and")};
  const QStringList words = title.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  QString initials;
  if (words.size() == 1) {
    for (const QChar letter : words.first()) {
      if (letter.isUpper()) {
        initials.append(letter);
      }
    }
    if (initials.size() < 2) {
      initials = words.first().left(2).toUpper();
    }
  } else {
    for (const QString& word : words) {
      if (!skipped.contains(word.toLower())) {
        initials.append(word.at(0).toUpper());
      }
    }
  }
  return initials.left(3);
}

Game gameFromWorld(const World& world)
{
  return {QStringLiteral("world%1").arg(world.serverID), world.serverID, world.game, world.version, world.name,
    initialsFor(world.game), world.color, world.status, world.accountDomainID, world.accountDomain, false};
}

void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint)
{
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(Qt::NoPen);
  painter.setBrush(tint.darker(210));
  const int radius = qMax(6, area.height() / 9);
  painter.drawRoundedRect(area, radius, radius);
  painter.setPen(QPen(QColor(255, 255, 255, 46), 1));
  painter.setBrush(Qt::NoBrush);
  painter.drawRoundedRect(QRectF(area).adjusted(0.5, 0.5, -0.5, -0.5), radius, radius);
  painter.setPen(tint.lighter(120));
  QFont font = painter.font();
  font.setBold(true);
  font.setPixelSize(qMax(10, area.height() / 3));
  painter.setFont(font);
  painter.drawText(area, Qt::AlignCenter, initials);
  painter.restore();
}
