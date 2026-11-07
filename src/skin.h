#pragma once

#include <QColor>
#include <QString>
#include <QStringList>

struct Skin {
  QString name;
  QColor base;
  QColor raised;
  QColor text;
  QColor secondary;
  QColor accent;
  QColor border;
  bool usesWordmarkImage;
};

const Skin& skinNamed(const QString& name);
QStringList skinNames();
QString buildStyleSheet(const Skin& skin);
