#include "gameCatalog.h"

#include "fatal.h"

#include <QFont>
#include <QPainter>

const QList<Game>& gameCatalog()
{
  static const QList<Game> games = {
    {QStringLiteral("everquestClassic"), QStringLiteral("EverQuest"), QStringLiteral("Classic"), QStringLiteral("EQ"), QColor("#D2A53A"), QString(), false, {}, {}, std::nullopt},
    {QStringLiteral("asheronsCallClassic"), QStringLiteral("Asheron's Call"), QStringLiteral("Classic"), QStringLiteral("AC"), QColor("#7C9A78"), QString(), false, {}, {}, std::nullopt},
    {QStringLiteral("wowClassic"), QStringLiteral("World of Warcraft"), QStringLiteral("Classic"), QStringLiteral("WoW"), QColor("#82B556"), QStringLiteral("1.12.1"), true,
      {QStringLiteral("Arcanite")}, {QStringLiteral("Main account")},
      Character{QStringLiteral("Merrin"), QStringLiteral("Level 60 Human Priest · Ironforge")}},
    {QStringLiteral("wowBurningCrusade"), QStringLiteral("World of Warcraft"), QStringLiteral("The Burning Crusade"), QStringLiteral("TBC"), QColor("#C9473A"), QString(), false, {}, {}, std::nullopt},
    {QStringLiteral("wowWrath"), QStringLiteral("World of Warcraft"), QStringLiteral("Wrath of the Lich King"), QStringLiteral("WLK"), QColor("#6FA8D6"), QString(), false, {}, {}, std::nullopt},
    {QStringLiteral("warhammerOnline"), QStringLiteral("Warhammer Online"), QStringLiteral("Age of Reckoning"), QStringLiteral("WAR"), QColor("#A04848"), QString(), false, {}, {}, std::nullopt},
    {QStringLiteral("motorCityOnline"), QStringLiteral("Motor City Online"), QStringLiteral("Original"), QStringLiteral("MCO"), QColor("#C0392B"), QString(), false, {}, {}, std::nullopt},
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
