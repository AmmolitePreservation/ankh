#pragma once

#include "ammoliteClient.h"

#include <QMap>
#include <QString>
#include <QUrl>
#include <optional>

struct UpdateOffer {
  QString version;
  QString notes;
  QUrl page;
  QMap<QString, QUrl> assets;
};

struct ReleaseCheck {
  std::optional<UpdateOffer> update;
  QString problem;
};

ReleaseCheck checkRelease(const AnkhRelease& release, const QString& currentVersion);
