#pragma once

#include <QDialog>
#include <QUrl>

class UpdateSheet : public QDialog {
  Q_OBJECT

public:
  UpdateSheet(const QString& version, const QString& notes, const QUrl& page, QWidget* parent = nullptr);
};
