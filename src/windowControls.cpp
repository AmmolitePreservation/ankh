#include "windowControls.h"

#include "captionButton.h"
#include "icons.h"

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
  return QSize(32, 32);
}

void UpdateButton::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  QColor color("#2F9E5B");
  if (isDown()) {
    color = color.darker(125);
  } else if (underMouse() || hasFocus()) {
    color = color.lighter(130);
  }
  painter.drawPixmap(QRect(5, 5, 22, 22), iconPixmap(IconKind::download, 22, color, devicePixelRatioF()));
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
  layout->addSpacing(4);
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
