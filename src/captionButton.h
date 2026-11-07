#pragma once

#include <QAbstractButton>
#include <QColor>

class CaptionButton : public QAbstractButton {
  Q_OBJECT

public:
  enum class Kind { minimize, maximize, restore, close };

  explicit CaptionButton(Kind kind, QWidget* parent = nullptr);

  void setKind(Kind newKind);
  void setColors(const QColor& glyph, const QColor& hover);
  void setNonClientState(bool isHovered, bool isPressed);
  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  Kind kind;
  QColor glyphColor;
  QColor hoverColor;
  bool isNonClientHovered = false;
  bool isNonClientPressed = false;
};
