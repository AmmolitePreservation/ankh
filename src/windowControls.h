#pragma once

#include "skin.h"

#include <QAbstractButton>
#include <QWidget>

class CaptionButton;

class UpdateButton : public QAbstractButton {
  Q_OBJECT

public:
  explicit UpdateButton(QWidget* parent = nullptr);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent* event) override;
};

class WindowControls : public QWidget {
  Q_OBJECT

public:
  static constexpr int barHeight = 32;

  explicit WindowControls(QWidget* parent = nullptr);

  UpdateButton* updateButton() const;
  CaptionButton* maximizeButton() const;
  void applySkin(const Skin& skin);
  void setMaximized(bool isMaximized);

signals:
  void minimizeRequested();
  void maximizeRequested();
  void closeRequested();
  void resized();

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  UpdateButton* update;
  CaptionButton* minimize;
  CaptionButton* maximize;
  CaptionButton* close;
};
