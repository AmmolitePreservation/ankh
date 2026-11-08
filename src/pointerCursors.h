#pragma once

#include <QObject>

class PointerCursors : public QObject {
  Q_OBJECT

public:
  explicit PointerCursors(QObject* parent = nullptr);

protected:
  bool eventFilter(QObject* watched, QEvent* event) override;
};
