#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;

class UpdateChecker : public QObject {
  Q_OBJECT

public:
  explicit UpdateChecker(const QString& currentVersion, QObject* parent = nullptr);

  void check();

signals:
  void checking();
  void updateFound(const QString& version, const QString& notes, const QUrl& page);
  void upToDate();
  void noReleases();
  void failed(const QString& message);

private:
  QString currentVersion;
  QNetworkAccessManager* network;
};
