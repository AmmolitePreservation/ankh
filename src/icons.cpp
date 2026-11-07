#include "icons.h"

#include "fatal.h"

#include <QFile>
#include <QHash>
#include <QPainter>
#include <QSvgRenderer>

namespace {

QString glyphName(IconKind kind)
{
  switch (kind) {
  case IconKind::search: return QStringLiteral("search");
  case IconKind::chevronDown: return QStringLiteral("chevronDown");
  case IconKind::chevronRight: return QStringLiteral("chevronRight");
  case IconKind::settings: return QStringLiteral("settings");
  case IconKind::activity: return QStringLiteral("activity");
  case IconKind::user: return QStringLiteral("user");
  case IconKind::star: return QStringLiteral("star");
  case IconKind::starFilled: return QStringLiteral("starFilled");
  case IconKind::checkBadge: return QStringLiteral("checkBadge");
  case IconKind::statusDot: return QStringLiteral("statusDot");
  case IconKind::play: return QStringLiteral("play");
  case IconKind::windowMinimize: return QStringLiteral("windowMinimize");
  case IconKind::windowMaximize: return QStringLiteral("windowMaximize");
  case IconKind::windowRestore: return QStringLiteral("windowRestore");
  case IconKind::windowClose: return QStringLiteral("windowClose");
  }
  fatal(QStringLiteral("Unknown icon kind"));
}

QByteArray glyphSource(IconKind kind)
{
  static QHash<QString, QByteArray> cache;
  const QString name = glyphName(kind);
  auto found = cache.constFind(name);
  if (found != cache.constEnd()) {
    return found.value();
  }
  QFile file(QStringLiteral(":/glyphs/%1.svg").arg(name));
  if (!file.open(QIODevice::ReadOnly)) {
    fatal(QStringLiteral("Missing glyph: %1").arg(name));
  }
  const QByteArray source = file.readAll();
  cache.insert(name, source);
  return source;
}

}

QPixmap iconPixmap(IconKind kind, int size, const QColor& color, qreal ratio)
{
  QByteArray source = glyphSource(kind);
  source.replace("currentColor", color.name().toLatin1());
  QSvgRenderer renderer(source);
  if (!renderer.isValid()) {
    fatal(QStringLiteral("Invalid glyph: %1").arg(glyphName(kind)));
  }
  QPixmap pixmap(QSize(size, size) * ratio);
  pixmap.setDevicePixelRatio(ratio);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  renderer.render(&painter, QRectF(0, 0, size, size));
  return pixmap;
}

QIcon makeIcon(IconKind kind, int size, const QColor& color)
{
  return QIcon(iconPixmap(kind, size, color, 2.0));
}
