#include "gameCatalog.h"

#include "fatal.h"

#include <QFont>
#include <QPainter>

const QList<Game>& gameCatalog()
{
  static const QList<Game> games = {
    {QStringLiteral("everquestClassic"), QStringLiteral("EverQuest"), QStringLiteral("Classic"), QStringLiteral("EQ"), QColor("#D2A53A"), false},
    {QStringLiteral("asheronsCallClassic"), QStringLiteral("Asheron's Call"), QStringLiteral("Classic"), QStringLiteral("AC"), QColor("#7C9A78"), false},
    {QStringLiteral("wowClassic"), QStringLiteral("World of Warcraft"), QStringLiteral("Classic"), QStringLiteral("WoW"), QColor("#82B556"), false},
    {QStringLiteral("wowBurningCrusade"), QStringLiteral("World of Warcraft"), QStringLiteral("The Burning Crusade"), QStringLiteral("TBC"), QColor("#C9473A"), false},
    {QStringLiteral("wowWrath"), QStringLiteral("World of Warcraft"), QStringLiteral("Wrath of the Lich King"), QStringLiteral("WLK"), QColor("#6FA8D6"), false},
    {QStringLiteral("warhammerOnline"), QStringLiteral("Warhammer Online"), QStringLiteral("Age of Reckoning"), QStringLiteral("WAR"), QColor("#A04848"), false},
    {QStringLiteral("motorCityOnline"), QStringLiteral("Motor City Online"), QStringLiteral("Original"), QStringLiteral("MCO"), QColor("#C0392B"), false},
  };
  return games;
}

const Game& gameWithId(const QString& id)
{
  for (const Game& game : gameCatalog()) {
    if (game.id == id) {
      return game;
    }
  }
  fatal(QStringLiteral("Unknown game id: %1").arg(id));
}

void paintGameIcon(QPainter& painter, const QRect& area, const QString& initials, const QColor& tint)
{
  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.setPen(Qt::NoPen);
  painter.setBrush(tint.darker(210));
  painter.drawRoundedRect(area, 8, 8);
  painter.setPen(tint.lighter(120));
  QFont font = painter.font();
  font.setBold(true);
  font.setPixelSize(qMax(10, area.height() / 3));
  painter.setFont(font);
  painter.drawText(area, Qt::AlignCenter, initials);
  painter.restore();
}
