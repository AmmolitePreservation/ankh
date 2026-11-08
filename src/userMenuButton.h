#pragma once

#include <QColor>
#include <QPushButton>

class UserMenuButton : public QPushButton {
  Q_OBJECT

public:
  UserMenuButton(const QString& name, QWidget* parent = nullptr);

  void setUserName(const QString& name);
  void setColors(const QColor& text, const QColor& secondary, const QColor& border);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QString userName;
  QColor textColor;
  QColor secondaryColor;
  QColor borderColor;
};
