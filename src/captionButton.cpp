#include "captionButton.h"

#include <QPainter>

CaptionButton::CaptionButton(Kind kind, QWidget* parent)
  : QAbstractButton(parent), kind(kind)
{
  setFocusPolicy(Qt::NoFocus);
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
  if (hasFocus()) {
    painter.setPen(QPen(glyphColor, 1, Qt::DotLine));
    painter.drawRect(rect().adjusted(3, 3, -4, -4));
  }
  painter.setPen(QPen(glyph, 1));
  const int centerX = width() / 2;
  const int centerY = height() / 2;
  switch (kind) {
  case Kind::minimize:
    painter.drawLine(centerX - 5, centerY, centerX + 4, centerY);
    break;
  case Kind::maximize:
    painter.drawRect(centerX - 5, centerY - 5, 9, 9);
    break;
  case Kind::restore:
    painter.drawRect(centerX - 5, centerY - 2, 7, 7);
    painter.drawLine(centerX - 3, centerY - 4, centerX + 4, centerY - 4);
    painter.drawLine(centerX + 4, centerY - 4, centerX + 4, centerY + 2);
    painter.drawLine(centerX + 3, centerY + 2, centerX + 2, centerY + 2);
    break;
  case Kind::close:
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawLine(centerX - 5, centerY - 5, centerX + 5, centerY + 5);
    painter.drawLine(centerX - 5, centerY + 5, centerX + 5, centerY - 5);
    break;
  }
}
