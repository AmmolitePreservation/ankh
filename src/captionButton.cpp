#include "captionButton.h"

#include "fatal.h"
#include "icons.h"

#include <QPainter>

namespace {

IconKind glyphFor(CaptionButton::Kind kind)
{
  switch (kind) {
  case CaptionButton::Kind::minimize: return IconKind::windowMinimize;
  case CaptionButton::Kind::maximize: return IconKind::windowMaximize;
  case CaptionButton::Kind::restore: return IconKind::windowRestore;
  case CaptionButton::Kind::close: return IconKind::windowClose;
  }
  fatal(QStringLiteral("Unknown caption button kind"));
}

}

CaptionButton::CaptionButton(Kind kind, QWidget* parent)
  : QAbstractButton(parent), kind(kind)
{
  setFocusPolicy(Qt::NoFocus);
  setProperty("systemCursor", true);
  setFixedSize(sizeHint());
}

void CaptionButton::setKind(Kind newKind)
{
  kind = newKind;
  update();
}

void CaptionButton::setColors(const QColor& glyph, const QColor& hover)
{
  glyphColor = glyph;
  hoverColor = hover;
  update();
}

void CaptionButton::setNonClientState(bool isHovered, bool isPressed)
{
  isNonClientHovered = isHovered;
  isNonClientPressed = isPressed;
  update();
}

QSize CaptionButton::sizeHint() const
{
  return QSize(46, 32);
}

void CaptionButton::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  const bool isHot = underMouse() || isNonClientHovered;
  const bool isHeldDown = isDown() || isNonClientPressed;
  QColor glyph = glyphColor;
  if (isHot) {
    QColor background = kind == Kind::close ? QColor("#C42B1C") : hoverColor;
    if (isHeldDown) {
      background = background.darker(115);
    }
    painter.fillRect(rect(), background);
    if (kind == Kind::close) {
      glyph = QColor("#FFFFFF");
    }
  }
  painter.drawPixmap(QRect(width() / 2 - 8, height() / 2 - 8, 16, 16), iconPixmap(glyphFor(kind), 16, glyph, devicePixelRatioF()));
}
