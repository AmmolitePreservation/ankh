#pragma once

#include <QMap>
#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

struct UpdateOffer {
  QString version;
  QString notes;
  QUrl page;
  QMap<QString, QUrl> assets;
};

class UpdateChecker : public QObject {
  Q_OBJECT

public:
  explicit UpdateChecker(const QString& currentVersion, QObject* parent = nullptr);

  void check();

signals:
  void checking();
  void updateFound(const UpdateOffer& offer);
  void upToDate();
  void noReleases();
  void failed(const QString& message);

private:
  QString currentVersion;
  QNetworkAccessManager* network;
};
