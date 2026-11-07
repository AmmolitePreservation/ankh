#include "titleBar.h"

#include "captionButton.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

UpdateButton::UpdateButton(QWidget* parent)
  : QAbstractButton(parent)
{
  setFocusPolicy(Qt::TabFocus);
  setCursor(Qt::PointingHandCursor);
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

TitleBar::TitleBar(QWidget* parent)
  : QWidget(parent),
    titleLabel(new QLabel(QStringLiteral("Ankh"), this)),
    update(new UpdateButton(this)),
    minimize(new CaptionButton(CaptionButton::Kind::minimize, this)),
    maximize(new CaptionButton(CaptionButton::Kind::maximize, this)),
    close(new CaptionButton(CaptionButton::Kind::close, this))
{
  setObjectName(QStringLiteral("titleBar"));
  setFixedHeight(barHeight);
  titleLabel->setProperty("secondary", true);
  update->setVisible(false);
  update->setToolTip(QStringLiteral("Update available"));
  update->setAccessibleName(QStringLiteral("Update available"));
  minimize->setAccessibleName(QStringLiteral("Minimize"));
  maximize->setAccessibleName(QStringLiteral("Maximize"));
  close->setAccessibleName(QStringLiteral("Close"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 0, 0, 0);
  layout->setSpacing(0);
  layout->addWidget(titleLabel);
  layout->addStretch(1);
  layout->addWidget(update, 0, Qt::AlignVCenter);
  layout->addSpacing(8);
  layout->addWidget(minimize);
  layout->addWidget(maximize);
  layout->addWidget(close);

  connect(minimize, &QAbstractButton::clicked, this, &TitleBar::minimizeRequested);
  connect(maximize, &QAbstractButton::clicked, this, &TitleBar::maximizeRequested);
  connect(close, &QAbstractButton::clicked, this, &TitleBar::closeRequested);
}

UpdateButton* TitleBar::updateButton() const
{
  return update;
}

CaptionButton* TitleBar::maximizeButton() const
{
  return maximize;
}

void TitleBar::applySkin(const Skin& skin)
{
  const QColor hover = skin.border;
  minimize->setColors(skin.text, hover);
  maximize->setColors(skin.text, hover);
  close->setColors(skin.text, hover);
}

void TitleBar::setMaximized(bool isMaximized)
{
  maximize->setKind(isMaximized ? CaptionButton::Kind::restore : CaptionButton::Kind::maximize);
  maximize->setAccessibleName(isMaximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
}

bool TitleBar::isCaptionAt(const QPoint& position) const
{
  const QWidget* child = childAt(position);
  return child == nullptr || child == titleLabel;
}

bool TitleBar::isMaximizeButtonAt(const QPoint& position) const
{
  return childAt(position) == maximize;
}
