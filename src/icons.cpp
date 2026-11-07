#include "icons.h"

#include <QPainter>
#include <QPolygonF>
#include <cmath>

QPixmap iconPixmap(IconKind kind, int size, const QColor& color, qreal ratio)
{
  QPixmap pixmap(QSize(size, size) * ratio);
  pixmap.setDevicePixelRatio(ratio);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.scale(size / 24.0, size / 24.0);
  painter.setPen(QPen(color, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  painter.setBrush(Qt::NoBrush);
  switch (kind) {
  case IconKind::search:
    painter.drawEllipse(QPointF(10.5, 10.5), 6.5, 6.5);
    painter.drawLine(QPointF(15.3, 15.3), QPointF(20.5, 20.5));
    break;
  case IconKind::chevronDown:
    painter.drawPolyline(QPolygonF({QPointF(6, 9), QPointF(12, 15), QPointF(18, 9)}));
    break;
  case IconKind::chevronRight:
    painter.drawPolyline(QPolygonF({QPointF(9, 6), QPointF(15, 12), QPointF(9, 18)}));
    break;
  case IconKind::settings:
    painter.drawEllipse(QPointF(12, 12), 3.2, 3.2);
    painter.drawEllipse(QPointF(12, 12), 7.2, 7.2);
    for (int tooth = 0; tooth < 8; ++tooth) {
      const qreal angle = tooth * M_PI / 4.0;
      painter.drawLine(QPointF(12 + 7.2 * std::cos(angle), 12 + 7.2 * std::sin(angle)),
        QPointF(12 + 10.2 * std::cos(angle), 12 + 10.2 * std::sin(angle)));
    }
    break;
  case IconKind::activity:
    painter.drawPolyline(QPolygonF({QPointF(2, 12), QPointF(7, 12), QPointF(10, 5), QPointF(14, 19), QPointF(17, 12), QPointF(22, 12)}));
    break;
  case IconKind::user:
    painter.drawEllipse(QPointF(12, 12), 10.2, 10.2);
    painter.drawEllipse(QPointF(12, 9.6), 3.3, 3.3);
    painter.drawArc(QRectF(6.6, 14.6, 10.8, 9), 0, 180 * 16);
    break;
  case IconKind::check:
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(12, 12), 11, 11);
    painter.setPen(QPen(QColor("#FFFFFF"), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(QPolygonF({QPointF(7, 12.5), QPointF(10.6, 16), QPointF(17, 8.6)}));
    break;
  case IconKind::dot:
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawEllipse(QPointF(12, 12), 8, 8);
    break;
  case IconKind::play:
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPolygon(QPolygonF({QPointF(7, 3.5), QPointF(20.5, 12), QPointF(7, 20.5)}));
    break;
  }
  return pixmap;
}

QIcon makeIcon(IconKind kind, int size, const QColor& color)
{
  return QIcon(iconPixmap(kind, size, color, 2.0));
}
