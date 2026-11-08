#include "windowControls.h"

#include "captionButton.h"

#include <QHBoxLayout>
#include <QPainter>

UpdateButton::UpdateButton(QWidget* parent)
  : QAbstractButton(parent)
{
  setFocusPolicy(Qt::TabFocus);
  setFixedSize(sizeHint());
}

QSize UpdateButton::sizeHint() const
{
  return QSize(40, 24);
}

void UpdateButton::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);
  QColor fill("#2F9E5B");
  if (isDown()) {
    fill = fill.darker(120);
  } else if (underMouse() || hasFocus()) {
    fill = fill.lighter(115);
  }
  painter.setPen(Qt::NoPen);
  painter.setBrush(fill);
  painter.drawRoundedRect(rect(), 4, 4);
  painter.setPen(QPen(QColor("#FFFFFF"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  const qreal centerX = width() / 2.0;
  painter.drawLine(QPointF(centerX, 6), QPointF(centerX, 14.5));
  painter.drawLine(QPointF(centerX - 4, 10.5), QPointF(centerX, 14.5));
  painter.drawLine(QPointF(centerX + 4, 10.5), QPointF(centerX, 14.5));
  painter.drawLine(QPointF(centerX - 5.5, 18), QPointF(centerX + 5.5, 18));
}

WindowControls::WindowControls(QWidget* parent)
  : QWidget(parent),
    update(new UpdateButton(this)),
    minimize(new CaptionButton(CaptionButton::Kind::minimize, this)),
    maximize(new CaptionButton(CaptionButton::Kind::maximize, this)),
    close(new CaptionButton(CaptionButton::Kind::close, this))
{
  setObjectName(QStringLiteral("windowControls"));
  setFixedHeight(barHeight);
  update->setVisible(false);
  update->setToolTip(QStringLiteral("Update available"));
  update->setAccessibleName(QStringLiteral("Update available"));
  minimize->setAccessibleName(QStringLiteral("Minimize"));
  maximize->setAccessibleName(QStringLiteral("Maximize"));
  close->setAccessibleName(QStringLiteral("Close"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);
  layout->setSizeConstraint(QLayout::SetFixedSize);
  layout->addWidget(update, 0, Qt::AlignVCenter);
  layout->addSpacing(8);
  layout->addWidget(minimize);
  layout->addWidget(maximize);
  layout->addWidget(close);

  connect(minimize, &QAbstractButton::clicked, this, &WindowControls::minimizeRequested);
  connect(maximize, &QAbstractButton::clicked, this, &WindowControls::maximizeRequested);
  connect(close, &QAbstractButton::clicked, this, &WindowControls::closeRequested);
}

UpdateButton* WindowControls::updateButton() const
{
  return update;
}

CaptionButton* WindowControls::maximizeButton() const
{
  return maximize;
}

void WindowControls::applyPalette(const Palette& colors)
{
  minimize->setColors(colors.text, colors.border);
  maximize->setColors(colors.text, colors.border);
  close->setColors(colors.text, colors.border);
}

void WindowControls::setMaximized(bool isMaximized)
{
  maximize->setKind(isMaximized ? CaptionButton::Kind::restore : CaptionButton::Kind::maximize);
  maximize->setAccessibleName(isMaximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
}

void WindowControls::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  emit resized();
}
