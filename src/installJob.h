#pragma once

#include "ammoliteClient.h"
#include "gameCatalog.h"
#include "installLibrary.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QUrl>
#include <functional>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class FileHasher;

struct InstallPlan {
  Package client;
  QList<Package> tools;
  QString folder;

  QList<Package> packages() const;
  qint64 downloadBytes() const;
  qint64 extractedBytes() const;
  int fileCount() const;
};

std::optional<InstallPlan> planInstall(const Game& game, const QList<Package>& packages, const QString& libraryRoot,
  QString& problem);
QString stagingFolderFor(const QString& libraryRoot);
QString stagedZipPath(const QString& libraryRoot, const Package& package);
qint64 bytesStillNeeded(const InstallPlan& plan, const QString& libraryRoot);

class InstallJob : public QObject {
  Q_OBJECT

public:
  InstallJob(const InstallPlan& plan, const QString& libraryRoot, const QUrl& serviceUrl, const QString& token,
    const QString& appVersion, QObject* parent = nullptr);

  const InstallPlan& plan() const;
  void start();
  void cancel();

signals:
  void statusChanged(const QString& text);
  void progressChanged(qint64 done, qint64 total);
  void finished(const Receipt& receipt);
  void failed(const QString& message);
  void cancelled();

private:
  void fetchListing(int index);
  void download(int index);
  void verifyDownload(int index);
  void extract(int index);
  void apply();
  void fail(const QString& message);
  bool stopIfCancelled();
  QString partPath(const Package& package) const;
  QString extractFolder(const Package& package) const;
  void removeExtractFolders();

  InstallPlan installPlan;
  QString libraryRoot;
  QUrl serviceUrl;
  QString token;
  QString appVersion;
  QNetworkAccessManager* network;
  QList<Package> queue;
  QMap<QString, OwnedFile> ownedFiles;
  qint64 downloadedBefore = 0;
  QNetworkReply* activeReply = nullptr;
  QProcess* activeProcess = nullptr;
  FileHasher* activeHasher = nullptr;
  bool isCancelled = false;
  bool isApplying = false;
};
