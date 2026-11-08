#include "pointerCursors.h"

#include <QAbstractButton>
#include <QComboBox>
#include <QEvent>
#include <QLineEdit>
#include <QTabBar>
#include <QWidget>

namespace {

bool isClickable(const QWidget* widget)
{
  if (widget->property("systemCursor").toBool()) {
    return false;
  }
  if (const auto* button = qobject_cast<const QAbstractButton*>(widget)) {
    if (const auto* line = qobject_cast<const QLineEdit*>(button->parentWidget())) {
      return button->x() >= line->width() / 2;
    }
    return true;
  }
  return qobject_cast<const QComboBox*>(widget) != nullptr || qobject_cast<const QTabBar*>(widget) != nullptr;
}

}

PointerCursors::PointerCursors(QObject* parent)
  : QObject(parent)
{
}

bool PointerCursors::eventFilter(QObject* watched, QEvent* event)
{
  if (event->type() == QEvent::Enter || event->type() == QEvent::EnabledChange) {
    auto* widget = qobject_cast<QWidget*>(watched);
    if (widget != nullptr && isClickable(widget)) {
      if (widget->isEnabled()) {
        widget->setCursor(Qt::PointingHandCursor);
      } else {
        widget->unsetCursor();
      }
    }
  }
  return false;
}
