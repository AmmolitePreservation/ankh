#include "userMenuButton.h"

#include "icons.h"

#include <QFontMetrics>
#include <QPainter>

UserMenuButton::UserMenuButton(const QString& name, QWidget* parent)
  : QPushButton(parent)
{
  setFlat(true);
  setFixedHeight(48);
  setUserName(name);
}

void UserMenuButton::setUserName(const QString& name)
{
  userName = name;
  setAccessibleName(QStringLiteral("%1 menu").arg(name));
  QFont measured = font();
  measured.setPixelSize(18);
  setFixedWidth(54 + QFontMetrics(measured).horizontalAdvance(name) + 46);
  update();
}

void UserMenuButton::setColors(const QColor& text, const QColor& secondary, const QColor& border)
{
  textColor = text;
  secondaryColor = secondary;
  borderColor = border;
  update();
}

void UserMenuButton::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  const qreal ratio = devicePixelRatioF();
  painter.setPen(QPen(QColor("#4A4E50"), 1));
  painter.setBrush(Qt::NoBrush);
  painter.drawEllipse(QRectF(0.5, 4.5, 39, 39));
  painter.drawPixmap(QRect(9, 13, 22, 22), iconPixmap(IconKind::user, 22, textColor, ratio));
  painter.setPen(textColor);
  QFont textFont = font();
  textFont.setPixelSize(18);
  painter.setFont(textFont);
  painter.drawText(QRect(54, 0, width() - 54 - 34, height()), Qt::AlignVCenter | Qt::AlignLeft, userName);
  painter.drawPixmap(QRect(width() - 26, 15, 18, 18), iconPixmap(IconKind::chevronDown, 18, textColor, ratio));
  if (hasFocus()) {
    painter.setPen(QPen(borderColor, 1, Qt::DotLine));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
  }
}
