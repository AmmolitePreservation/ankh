#pragma once

#include "skin.h"

#include <QAbstractButton>
#include <QWidget>

class CaptionButton;
class QLabel;

class UpdateButton : public QAbstractButton {
  Q_OBJECT

public:
  explicit UpdateButton(QWidget* parent = nullptr);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent* event) override;
};

class TitleBar : public QWidget {
  Q_OBJECT

public:
  static constexpr int barHeight = 32;

  explicit TitleBar(QWidget* parent = nullptr);

  UpdateButton* updateButton() const;
  CaptionButton* maximizeButton() const;
  void applySkin(const Skin& skin);
  void setMaximized(bool isMaximized);
  bool isCaptionAt(const QPoint& position) const;
  bool isMaximizeButtonAt(const QPoint& position) const;

signals:
  void minimizeRequested();
  void maximizeRequested();
  void closeRequested();

private:
  QLabel* titleLabel;
  UpdateButton* update;
  CaptionButton* minimize;
  CaptionButton* maximize;
  CaptionButton* close;
};
