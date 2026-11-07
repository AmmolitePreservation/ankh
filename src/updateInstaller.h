#pragma once

#include "updateChecker.h"

#include <QCryptographicHash>
#include <QFile>
#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QWinEventNotifier;

void removeUpdateLeftovers();

class UpdateInstaller : public QObject {
  Q_OBJECT

public:
  UpdateInstaller(const UpdateOffer& offer, const QString& currentVersion, QObject* parent = nullptr);

  void start();

signals:
  void statusChanged(const QString& text);
  void progressChanged(qint64 received, qint64 total);
  void failed(const QString& message);
  void installerStarted();

private:
  void fetch(const QString& assetName, std::function<void(const QByteArray&)> onFinished);
  void verifyManifest(const QByteArray& manifest, const QByteArray& signature);
  void downloadInstaller(const QString& assetName, qint64 size, const QString& sha256);
  void launchInstaller(const QString& installerPath);

  UpdateOffer offer;
  QString currentVersion;
  QString workFolder;
  QNetworkAccessManager* network;
  QWinEventNotifier* processWatcher = nullptr;
};
