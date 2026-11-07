#include "edgeGrain.h"

#include "fatal.h"

#include <QPainter>
#include <QTransform>

namespace {

QPixmap loadTexture(const QString& path)
{
  QPixmap pixmap(path);
  if (pixmap.isNull()) {
    fatal(QStringLiteral("Missing texture: %1").arg(path));
  }
  return pixmap;
}

}

EdgeGrain::EdgeGrain(QWidget* parent)
  : QWidget(parent),
    strip(loadTexture(QStringLiteral(":/textures/edgeGrainStrip.png"))),
    corner(loadTexture(QStringLiteral(":/textures/edgeGrainCorner.png")))
{
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_NoSystemBackground);
  setFocusPolicy(Qt::NoFocus);
}

void EdgeGrain::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  const int length = width();
  const int stripLength = strip.height();

  painter.setOpacity(0.5);
  for (int y = 0; y < height(); y += stripLength) {
    painter.drawPixmap(0, y, strip);
  }
  painter.setOpacity(0.55);
  painter.drawPixmap(0, 0, corner);

  painter.setOpacity(0.18);
  const QPixmap top = strip.transformed(QTransform().rotate(90));
  for (int x = 0; x < length; x += top.width()) {
    painter.drawPixmap(x, 0, top);
  }

  painter.setOpacity(0.14);
  const QPixmap right = strip.transformed(QTransform().scale(-1, 1));
  for (int y = 0; y < height(); y += stripLength) {
    painter.drawPixmap(width() - right.width(), y, right);
  }
  const QPixmap bottom = strip.transformed(QTransform().rotate(-90));
  for (int x = 0; x < length; x += bottom.width()) {
    painter.drawPixmap(x, height() - bottom.height(), bottom);
  }
}
