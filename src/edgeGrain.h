#pragma once

#include <QPixmap>
#include <QWidget>

class EdgeGrain : public QWidget {
  Q_OBJECT

public:
  explicit EdgeGrain(QWidget* parent);

protected:
  void paintEvent(QPaintEvent* event) override;

private:
  QPixmap strip;
  QPixmap corner;
};
