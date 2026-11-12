#include "gameCatalog.h"

#include "fatal.h"

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
  return {QStringLiteral("world%1").arg(world.serverID), world.serverID, world.game, world.version, world.variant,
    world.name, initialsFor(world.game), world.color, world.status, world.accountDomainID, world.accountDomain, false};
}

Game gameFromAttached(const AttachedGame& attached)
{
  return {QStringLiteral("attached/%1/%2").arg(attached.game, attached.variant), 0, attached.game, attached.edition,
    attached.variant, QString(), initialsFor(attached.game), attached.color, QString(), std::nullopt, QString(), false, true,
    attached.executable, attached.launcherConfig, attached.steamAppID};
}

QUrl steamStartUrl(const Game& game)
{
  return QUrl(QStringLiteral("steam://rungameid/%1").arg(game.steamAppID));
}

QList<Package> packagesFor(const Game& game, const QList<Package>& packages)
{
  QList<Package> attached;
  for (const Package& package : packages) {
    for (const Edition& edition : package.editions) {
      if (edition.game == game.title && edition.variant == game.variant) {
        attached.append(package);
        break;
      }
    }
  }
  return attached;
}

QString roleLabel(const QString& role)
{
  if (role == QLatin1String("client")) {
    return QStringLiteral("Client");
  }
  if (role == QLatin1String("requiredPlugin")) {
    return QStringLiteral("Required plugins");
  }
  if (role == QLatin1String("optionalPlugin")) {
    return QStringLiteral("Optional plugins");
  }
  fatal(QStringLiteral("Unknown package role: %1").arg(role));
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
