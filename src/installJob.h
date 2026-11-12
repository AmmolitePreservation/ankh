#pragma once

#include "ammoliteClient.h"
#include "gameCatalog.h"
#include "installLibrary.h"

#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <functional>
#include <optional>

class QNetworkAccessManager;
class QNetworkReply;
class QProcess;
class FileHasher;

// What one install or update does: fetch holds the packages downloaded and extracted (every package on a fresh
// install, the changed ones on an update), removed the receipt's packages that left the edition, existing the
// receipt an update starts from. plugins is what the receipt lists afterwards.
struct InstallPlan {
  Package client;
  QList<Package> plugins;
  QString folder;
  QList<Package> fetch;
  QStringList removed;
  std::optional<Receipt> existing;
  bool isPluginChange = false;
  bool isAttach = false;

  bool isUpdate() const;
  qint64 downloadBytes() const;
  qint64 extractedBytes() const;
  int fileCount() const;
};

struct ApplySummary {
  int written = 0;
  int deleted = 0;
  QStringList kept;
  // Files written over that were already in the folder and that no earlier receipt listed.
  QStringList replaced;
};

// optionalPluginNames are the optional plugins installed with the game; every required plugin always is.
std::optional<InstallPlan> planInstall(const Game& game, const QList<Package>& packages, const QString& parentFolder,
  QString& problem, const QStringList& optionalPluginNames = {});
// Brings the installed optional plugins to exactly optionalPluginNames: missing ones are fetched, unwanted ones removed
// (their write-once files stay). The client and the plugins already installed are left at the versions they have; a
// required plugin that is missing is fetched. Nothing to do is nullopt with an empty problem.
std::optional<InstallPlan> planPluginChange(const Game& game, const QList<Package>& packages, const Receipt& receipt,
  const QStringList& optionalPluginNames, QString& problem);
// The first install of an attached game's plugins into the player's own copy in gameFolder: every required plugin and the
// optional ones named. The folder must hold the game's executable.
std::optional<InstallPlan> planAttach(const Game& game, const QList<Package>& packages, const QString& gameFolder,
  QString& problem, const QStringList& optionalPluginNames = {});
std::optional<InstallPlan> planUpdate(const Game& game, const QList<Package>& packages, const Receipt& receipt,
  QString& problem);
QString stagingRootFor(const InstallPlan& plan);
QString stagingFolderFor(const QString& stagingRoot);
QString stagedZipPath(const QString& stagingRoot, const Package& package);
qint64 bytesStillNeeded(const InstallPlan& plan, const QString& stagingRoot);

class InstallJob : public QObject {
  Q_OBJECT

public:
  InstallJob(const InstallPlan& plan, const QUrl& serviceUrl, const QString& token, const QString& appVersion,
    QObject* parent = nullptr);

  const InstallPlan& plan() const;
  void start();
  void cancel();

signals:
  void statusChanged(const QString& text);
  void progressChanged(qint64 done, qint64 total);
  void finished(const Receipt& receipt, const ApplySummary& summary);
  void failed(const QString& message);
  void cancelled();

private:
  void fetchListing(int index);
  void download(int index);
  void verifyDownload(int index);
  void extract(int index);
  void apply();
  void applyInstall();
  void applyUpdate();
  void complete(Receipt receipt, const ApplySummary& summary);
  void fail(const QString& message);
  bool stopIfCancelled();
  QString partPath(const Package& package) const;
  QString extractFolder(const Package& package) const;
  const Package& fetchedPackage(const QString& name) const;
  void removeExtractFolders();

  InstallPlan installPlan;
  QString stagingRoot;
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
