#include "installJob.h"

#include "fatal.h"

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSet>
#include <QStorageInfo>
#include <QTimer>
#include <algorithm>

namespace {

constexpr qint64 hashChunkBytes = 16 * 1024 * 1024;

QString sizeText(qint64 bytes)
{
  return QLocale().formattedDataSize(bytes, 1, QLocale::DataSizeTraditionalFormat);
}

QString tarPath()
{
  const QString path = QDir(qEnvironmentVariable("SystemRoot")).filePath(QStringLiteral("System32/tar.exe"));
  if (!QFile::exists(path)) {
    fatal(QStringLiteral("Windows' tar.exe is missing at %1.").arg(QDir::toNativeSeparators(path)));
  }
  return path;
}

QString native(const QString& path)
{
  return QDir::toNativeSeparators(path);
}

struct ListingError {
  QString message;
};

QMap<QString, OwnedFile> parseListing(const QByteArray& payload, const QString& packageName)
{
  const QJsonDocument document = QJsonDocument::fromJson(payload);
  if (!document.isObject() || !document.object().value(QStringLiteral("files")).isArray()) {
    throw ListingError{QStringLiteral("the file list of %1 was not understood").arg(packageName)};
  }
  QMap<QString, OwnedFile> files;
  for (const QJsonValue& value : document.object().value(QStringLiteral("files")).toArray()) {
    const QJsonObject entry = value.toObject();
    const QJsonValue path = entry.value(QStringLiteral("path"));
    const QJsonValue byteSize = entry.value(QStringLiteral("byteSize"));
    const QJsonValue crc = entry.value(QStringLiteral("crc32"));
    const QJsonValue writeOnce = entry.value(QStringLiteral("writeOnce"));
    if (!path.isString() || path.toString().isEmpty() || !byteSize.isDouble() || !crc.isDouble() || !writeOnce.isBool()
        || byteSize.toInteger() < 0 || crc.toInteger() < 0) {
      throw ListingError{QStringLiteral("the file list of %1 has an entry that was not understood").arg(packageName)};
    }
    files.insert(path.toString(), {packageName, byteSize.toInteger(), static_cast<quint32>(crc.toInteger()), writeOnce.toBool()});
  }
  return files;
}

const Package* clientOf(const QList<Package>& attached)
{
  for (const Package& package : attached) {
    if (package.role == QLatin1String("client")) {
      return &package;
    }
  }
  return nullptr;
}

}

// Hashes a file in chunks between event loop turns, so a multi-gigabyte download is checked without freezing the
// window.
class FileHasher : public QObject {
public:
  FileHasher(const QString& path, QObject* parent)
    : QObject(parent), file(path), hash(QCryptographicHash::Sha256)
  {
  }

  void start(std::function<void(const QString&)> onDone, std::function<void(qint64, qint64)> onProgress,
    std::function<void(const QString&)> onError)
  {
    done = std::move(onDone);
    progress = std::move(onProgress);
    error = std::move(onError);
    if (!file.open(QIODevice::ReadOnly)) {
      error(file.errorString());
      return;
    }
    total = file.size();
    step();
  }

  void stop()
  {
    isStopped = true;
  }

private:
  void step()
  {
    if (isStopped) {
      return;
    }
    const QByteArray chunk = file.read(hashChunkBytes);
    if (chunk.isEmpty() && !file.atEnd()) {
      error(file.errorString());
      return;
    }
    hash.addData(chunk);
    read += chunk.size();
    progress(read, total);
    if (file.atEnd()) {
      file.close();
      done(QString::fromLatin1(hash.result().toHex()));
      return;
    }
    QTimer::singleShot(0, this, [this] { step(); });
  }

  QFile file;
  QCryptographicHash hash;
  qint64 total = 0;
  qint64 read = 0;
  bool isStopped = false;
  std::function<void(const QString&)> done;
  std::function<void(qint64, qint64)> progress;
  std::function<void(const QString&)> error;
};

bool InstallPlan::isUpdate() const
{
  return existing.has_value();
}

qint64 InstallPlan::downloadBytes() const
{
  qint64 total = 0;
  for (const Package& package : fetch) {
    total += package.latest->byteSize;
  }
  return total;
}

qint64 InstallPlan::extractedBytes() const
{
  qint64 total = 0;
  for (const Package& package : fetch) {
    total += package.latest->extractedBytes;
  }
  return total;
}

int InstallPlan::fileCount() const
{
  int total = 0;
  for (const Package& package : fetch) {
    total += package.latest->fileCount;
  }
  return total;
}

std::optional<InstallPlan> planInstall(const Game& game, const QList<Package>& packages, const QString& parentFolder,
  QString& problem, const QStringList& optionalToolNames)
{
  problem.clear();
  const QList<Package> attached = packagesFor(game, packages);
  const Package* client = clientOf(attached);
  if (client == nullptr) {
    problem = QStringLiteral("No client is published for %1 %2 yet.").arg(game.title, game.edition);
    return std::nullopt;
  }
  if (!client->latest) {
    problem = QStringLiteral("The client %1 has no version published yet.").arg(client->label);
    return std::nullopt;
  }
  InstallPlan plan;
  plan.client = *client;
  plan.fetch.append(*client);
  for (const Package& package : attached) {
    const bool isRequired = package.role == QLatin1String("requiredTool");
    if (!isRequired && !(package.role == QLatin1String("optionalTool") && optionalToolNames.contains(package.name))) {
      continue;
    }
    if (!package.latest) {
      problem = QStringLiteral("The %1 %2 has no version published yet.")
        .arg(isRequired ? QStringLiteral("required tool") : QStringLiteral("optional tool"), package.label);
      return std::nullopt;
    }
    plan.tools.append(package);
    plan.fetch.append(package);
  }
  for (const QString& name : optionalToolNames) {
    const bool isAttached = std::any_of(attached.cbegin(), attached.cend(), [&name](const Package& package) {
      return package.name == name && package.role == QLatin1String("optionalTool");
    });
    if (!isAttached) {
      problem = QStringLiteral("%1 is not an optional tool of %2 %3.").arg(name, game.title, game.edition);
      return std::nullopt;
    }
  }
  plan.folder = native(QDir(parentFolder).filePath(installFolderNameFor(plan.client)));
  return plan;
}

std::optional<InstallPlan> planAttach(const Game& game, const QList<Package>& packages, const QString& gameFolder,
  QString& problem, const QStringList& optionalToolNames)
{
  problem.clear();
  if (!game.isAttached) {
    fatal(QStringLiteral("%1 %2 is installed, not attached.").arg(game.title, game.edition));
  }
  const QDir folder(gameFolder);
  if (!QFileInfo(folder.filePath(game.executable)).isFile()) {
    problem = QStringLiteral("%1 is not a copy of %2: it has no %3.")
      .arg(native(folder.absolutePath()), game.title, native(game.executable));
    return std::nullopt;
  }
  Receipt empty{native(folder.absolutePath()), {}, {}, {}, game.title, game.variant};
  std::optional<InstallPlan> plan = planToolChange(game, packages, empty, optionalToolNames, problem);
  if (!plan) {
    if (problem.isEmpty()) {
      problem = QStringLiteral("No tools are published for %1 %2 yet.").arg(game.title, game.edition);
    }
    return std::nullopt;
  }
  plan->isAttach = true;
  return plan;
}

std::optional<InstallPlan> planToolChange(const Game& game, const QList<Package>& packages, const Receipt& receipt,
  const QStringList& optionalToolNames, QString& problem)
{
  problem.clear();
  const QList<Package> attached = packagesFor(game, packages);
  const Package* client = clientOf(attached);
  if (receipt.isAttached() != game.isAttached
      || (!game.isAttached && (client == nullptr || client->name != receipt.client.name))) {
    problem = QStringLiteral("The client of %1 %2 is not the one installed in %3.").arg(game.title, game.edition, receipt.folder);
    return std::nullopt;
  }
  InstallPlan plan;
  if (client != nullptr) {
    plan.client = *client;
  }
  plan.folder = receipt.folder;
  plan.existing = receipt;
  plan.isToolChange = true;
  QSet<QString> installedNames;
  for (const InstalledPackage& tool : receipt.tools) {
    installedNames.insert(tool.name);
  }
  QSet<QString> attachedNames;
  for (const Package& package : attached) {
    attachedNames.insert(package.name);
    const bool isRequired = package.role == QLatin1String("requiredTool");
    if (package.role == QLatin1String("client")) {
      continue;
    }
    const bool isWanted = isRequired || optionalToolNames.contains(package.name);
    const bool isInstalled = installedNames.contains(package.name);
    if (isWanted && !isInstalled && !package.latest) {
      problem = QStringLiteral("The %1 %2 has no version published yet.")
        .arg(isRequired ? QStringLiteral("required tool") : QStringLiteral("optional tool"), package.label);
      return std::nullopt;
    }
    if (isWanted) {
      plan.tools.append(package);
      if (!isInstalled) {
        plan.fetch.append(package);
      }
    } else if (isInstalled) {
      plan.removed.append(package.name);
    }
  }
  for (const QString& name : optionalToolNames) {
    if (!attachedNames.contains(name)) {
      problem = QStringLiteral("%1 is not a tool of %2 %3.").arg(name, game.title, game.edition);
      return std::nullopt;
    }
  }
  for (const InstalledPackage& tool : receipt.tools) {
    if (!attachedNames.contains(tool.name)) {
      plan.removed.append(tool.name);
    }
  }
  if (plan.fetch.isEmpty() && plan.removed.isEmpty()) {
    return std::nullopt;
  }
  return plan;
}

// Staging sits beside the install folder, so moving files into place never crosses a drive.
QString stagingRootFor(const InstallPlan& plan)
{
  if (plan.existing && plan.existing->isAttached()) {
    // The folder beside a game the player installed themselves is not Ankh's to write in.
    return native(QDir(plan.folder).filePath(QStringLiteral(".ankh")));
  }
  return native(QFileInfo(plan.folder).absolutePath());
}

std::optional<InstallPlan> planUpdate(const Game& game, const QList<Package>& packages, const Receipt& receipt,
  QString& problem)
{
  problem.clear();
  const QList<Package> attached = packagesFor(game, packages);
  const Package* client = clientOf(attached);
  InstallPlan plan;
  plan.folder = receipt.folder;
  plan.existing = receipt;
  if (receipt.isAttached() != game.isAttached) {
    problem = QStringLiteral("%1 holds a different kind of install than %2 %3.").arg(receipt.folder, game.title, game.edition);
    return std::nullopt;
  }
  if (!game.isAttached) {
    if (client == nullptr) {
      problem = QStringLiteral("No client is published for %1 %2 any more.").arg(game.title, game.edition);
      return std::nullopt;
    }
    if (client->name != receipt.client.name) {
      problem = QStringLiteral("The edition's client changed from %1 to %2. Remove %3 and install again.")
        .arg(receipt.client.name, client->name, receipt.folder);
      return std::nullopt;
    }
    if (!client->latest) {
      problem = QStringLiteral("The client %1 has no version published any more.").arg(client->label);
      return std::nullopt;
    }
    plan.client = *client;
    if (client->latest->version != receipt.client.version) {
      plan.fetch.append(*client);
    }
  }
  QMap<QString, QString> installedVersions;
  for (const InstalledPackage& tool : receipt.tools) {
    installedVersions.insert(tool.name, tool.version);
  }
  QSet<QString> attachedNames;
  for (const Package& package : attached) {
    attachedNames.insert(package.name);
    if (package.role == QLatin1String("client")) {
      continue;
    }
    const auto installed = installedVersions.constFind(package.name);
    if (package.role == QLatin1String("requiredTool")) {
      if (!package.latest) {
        problem = QStringLiteral("The required tool %1 has no version published yet.").arg(package.label);
        return std::nullopt;
      }
      plan.tools.append(package);
      if (installed == installedVersions.constEnd() || *installed != package.latest->version) {
        plan.fetch.append(package);
      }
    } else if (installed != installedVersions.constEnd()) {
      // An optional tool that is installed stays and follows its newest version.
      plan.tools.append(package);
      if (package.latest && package.latest->version != *installed) {
        plan.fetch.append(package);
      }
    }
  }
  for (const InstalledPackage& tool : receipt.tools) {
    if (!attachedNames.contains(tool.name)) {
      plan.removed.append(tool.name);
    }
  }
  if (plan.fetch.isEmpty() && plan.removed.isEmpty()) {
    return std::nullopt;
  }
  return plan;
}

QString stagingFolderFor(const QString& stagingRoot)
{
  return native(QDir(stagingRoot).filePath(QStringLiteral(".staging")));
}

QString stagedZipPath(const QString& stagingRoot, const Package& package)
{
  return native(QDir(stagingFolderFor(stagingRoot)).filePath(QStringLiteral("%1-%2.zip").arg(package.name, package.latest->version)));
}

qint64 bytesStillNeeded(const InstallPlan& plan, const QString& stagingRoot)
{
  qint64 needed = plan.extractedBytes();
  for (const Package& package : plan.fetch) {
    const QString zip = stagedZipPath(stagingRoot, package);
    if (QFile::exists(zip)) {
      continue;
    }
    needed += qMax<qint64>(0, package.latest->byteSize - QFileInfo(zip + QStringLiteral(".part")).size());
  }
  return needed;
}

InstallJob::InstallJob(const InstallPlan& plan, const QUrl& serviceUrl, const QString& token, const QString& appVersion,
  QObject* parent)
  : QObject(parent), installPlan(plan), stagingRoot(stagingRootFor(plan)), serviceUrl(serviceUrl), token(token),
    appVersion(appVersion), network(new QNetworkAccessManager(this)), queue(plan.fetch)
{
}

const InstallPlan& InstallJob::plan() const
{
  return installPlan;
}

QString InstallJob::partPath(const Package& package) const
{
  return stagedZipPath(stagingRoot, package) + QStringLiteral(".part");
}

QString InstallJob::extractFolder(const Package& package) const
{
  return native(QDir(stagingFolderFor(stagingRoot)).filePath(QStringLiteral("%1-%2.extract").arg(package.name, package.latest->version)));
}

const Package& InstallJob::fetchedPackage(const QString& name) const
{
  for (const Package& package : queue) {
    if (package.name == name) {
      return package;
    }
  }
  fatal(QStringLiteral("The install plan has no package named %1.").arg(name));
}

void InstallJob::start()
{
  if (!QDir().mkpath(stagingFolderFor(stagingRoot))) {
    fail(QStringLiteral("Could not create %1.").arg(stagingFolderFor(stagingRoot)));
    return;
  }
  if (!installPlan.isUpdate() && QDir(installPlan.folder).exists()) {
    fail(QStringLiteral("%1 already exists. Remove it before installing.").arg(installPlan.folder));
    return;
  }
  if (installPlan.isUpdate() && !QDir(installPlan.folder).exists()) {
    fail(QStringLiteral("%1 no longer exists.").arg(installPlan.folder));
    return;
  }
  const QStorageInfo storage(stagingRoot);
  const qint64 needed = bytesStillNeeded(installPlan, stagingRoot);
  if (storage.bytesAvailable() < needed) {
    fail(QStringLiteral("Not enough space on %1: the install needs %2 and %3 is free.")
      .arg(native(storage.rootPath()), sizeText(needed), sizeText(storage.bytesAvailable())));
    return;
  }
  fetchListing(0);
}

void InstallJob::cancel()
{
  if (isCancelled || isApplying) {
    return;
  }
  isCancelled = true;
  if (activeReply != nullptr) {
    activeReply->abort();
  } else if (activeProcess != nullptr) {
    activeProcess->kill();
  } else if (activeHasher != nullptr) {
    activeHasher->stop();
    activeHasher = nullptr;
    stopIfCancelled();
  }
}

bool InstallJob::stopIfCancelled()
{
  if (!isCancelled) {
    return false;
  }
  removeExtractFolders();
  emit cancelled();
  return true;
}

void InstallJob::fail(const QString& message)
{
  removeExtractFolders();
  emit failed(message);
}

void InstallJob::removeExtractFolders()
{
  for (const Package& package : queue) {
    QDir(extractFolder(package)).removeRecursively();
  }
}

void InstallJob::fetchListing(int index)
{
  if (stopIfCancelled()) {
    return;
  }
  if (index >= queue.size()) {
    download(0);
    return;
  }
  const Package& package = queue.at(index);
  emit statusChanged(QStringLiteral("Reading the file list of %1...").arg(package.label));
  QUrl url = serviceUrl;
  url.setPath(QStringLiteral("/api/v1/packages/%1/versions/%2/files").arg(package.name, package.latest->version));
  QNetworkRequest request(url);
  request.setRawHeader("Accept", "application/json");
  request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(appVersion));
  request.setTransferTimeout(30000);
  QNetworkReply* reply = network->get(request);
  activeReply = reply;
  connect(reply, &QNetworkReply::finished, this, [this, reply, index, package] {
    reply->deleteLater();
    activeReply = nullptr;
    if (stopIfCancelled()) {
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      fail(QStringLiteral("Could not read the file list of %1: %2").arg(package.label, reply->errorString()));
      return;
    }
    try {
      ownedFiles.insert(parseListing(reply->readAll(), package.name));
    } catch (const ListingError& error) {
      fail(QStringLiteral("The server's answer was not understood: %1.").arg(error.message));
      return;
    }
    fetchListing(index + 1);
  });
}

void InstallJob::download(int index)
{
  if (stopIfCancelled()) {
    return;
  }
  if (index >= queue.size()) {
    extract(0);
    return;
  }
  const Package& package = queue.at(index);
  const PackageVersion& latest = *package.latest;
  const QString zip = stagedZipPath(stagingRoot, package);
  if (QFile::exists(zip)) {
    downloadedBefore += latest.byteSize;
    download(index + 1);
    return;
  }
  const QString part = partPath(package);
  qint64 existing = QFileInfo(part).size();
  if (existing > latest.byteSize) {
    QFile::remove(part);
    existing = 0;
  }
  if (existing == latest.byteSize) {
    verifyDownload(index);
    return;
  }
  auto file = std::make_shared<QFile>(part);
  if (!file->open(QIODevice::WriteOnly | QIODevice::Append)) {
    fail(QStringLiteral("Could not write %1: %2").arg(part, file->errorString()));
    return;
  }
  emit statusChanged(QStringLiteral("Downloading %1 (%2)...").arg(package.label, sizeText(latest.byteSize)));
  QNetworkRequest request(latest.url);
  request.setRawHeader("Accept", "application/octet-stream");
  request.setRawHeader("Authorization", "Bearer " + token.toUtf8());
  request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Ankh/%1").arg(appVersion));
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  request.setTransferTimeout(60000);
  if (existing > 0) {
    request.setRawHeader("Range", "bytes=" + QByteArray::number(existing) + "-");
  }
  QNetworkReply* reply = network->get(request);
  activeReply = reply;
  auto received = std::make_shared<qint64>(existing);
  auto isRestarting = std::make_shared<bool>(false);
  connect(reply, &QNetworkReply::readyRead, this, [this, reply, file, received, existing, isRestarting] {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status != 200 && status != 206) {
      reply->readAll();
      return;
    }
    if (existing > 0 && status == 200 && !*isRestarting) {
      // The server ignored the range and is sending the whole file: what was kept cannot be appended to.
      *isRestarting = true;
      file->close();
      file->remove();
      reply->abort();
      return;
    }
    const QByteArray chunk = reply->readAll();
    if (file->write(chunk) != chunk.size()) {
      reply->abort();
      return;
    }
    *received += chunk.size();
    emit progressChanged(downloadedBefore + *received, installPlan.downloadBytes());
  });
  connect(reply, &QNetworkReply::finished, this, [this, reply, file, received, index, package, latest, isRestarting, part] {
    reply->deleteLater();
    activeReply = nullptr;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (file->isOpen()) {
      if (!*isRestarting && (status == 200 || status == 206)) {
        const QByteArray rest = reply->readAll();
        file->write(rest);
        *received += rest.size();
      }
      file->close();
    }
    if (*isRestarting) {
      download(index);
      return;
    }
    if (stopIfCancelled()) {
      return;
    }
    if (reply->error() != QNetworkReply::NoError) {
      if (status == 416 && QFileInfo(part).size() == latest.byteSize) {
        verifyDownload(index);
        return;
      }
      fail(QStringLiteral("Could not download %1: %2").arg(package.label, reply->errorString()));
      return;
    }
    if (QFileInfo(part).size() != latest.byteSize) {
      fail(QStringLiteral("The download of %1 ended at %2 of %3.").arg(package.label, sizeText(QFileInfo(part).size()),
        sizeText(latest.byteSize)));
      return;
    }
    verifyDownload(index);
  });
}

void InstallJob::verifyDownload(int index)
{
  const Package& package = queue.at(index);
  const PackageVersion& latest = *package.latest;
  const QString part = partPath(package);
  emit statusChanged(QStringLiteral("Checking %1...").arg(package.label));
  auto* hasher = new FileHasher(part, this);
  activeHasher = hasher;
  hasher->start(
    [this, hasher, index, package, latest, part](const QString& digest) {
      hasher->deleteLater();
      activeHasher = nullptr;
      if (stopIfCancelled()) {
        return;
      }
      if (digest != latest.sha256.toLower()) {
        QFile::remove(part);
        fail(QStringLiteral("The download of %1 does not match its published SHA-256, so it was deleted.").arg(package.label));
        return;
      }
      if (!QFile::rename(part, stagedZipPath(stagingRoot, package))) {
        fail(QStringLiteral("Could not keep the verified download of %1 in %2.").arg(package.label, stagingFolderFor(stagingRoot)));
        return;
      }
      downloadedBefore += latest.byteSize;
      download(index + 1);
    },
    [this](qint64 done, qint64 total) { emit progressChanged(done, total); },
    [this, hasher, package](const QString& message) {
      hasher->deleteLater();
      activeHasher = nullptr;
      fail(QStringLiteral("Could not read the download of %1: %2").arg(package.label, message));
    });
}

void InstallJob::extract(int index)
{
  if (stopIfCancelled()) {
    return;
  }
  if (index >= queue.size()) {
    apply();
    return;
  }
  const Package& package = queue.at(index);
  const QString folder = extractFolder(package);
  QDir(folder).removeRecursively();
  if (!QDir().mkpath(folder)) {
    fail(QStringLiteral("Could not create %1.").arg(folder));
    return;
  }
  emit statusChanged(QStringLiteral("Extracting %1...").arg(package.label));
  emit progressChanged(0, package.latest->fileCount);
  auto* process = new QProcess(this);
  activeProcess = process;
  auto extracted = std::make_shared<int>(0);
  auto errorText = std::make_shared<QString>();
  connect(process, &QProcess::readyReadStandardError, this, [this, process, extracted, errorText, package] {
    const QString text = QString::fromLocal8Bit(process->readAllStandardError());
    for (const QString& line : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
      if (line.startsWith(QLatin1String("x "))) {
        ++*extracted;
      } else {
        errorText->append(line.trimmed() + QLatin1Char(' '));
      }
    }
    emit progressChanged(*extracted, package.latest->fileCount);
  });
  connect(process, &QProcess::finished, this, [this, process, index, package, errorText](int exitCode, QProcess::ExitStatus status) {
    process->deleteLater();
    activeProcess = nullptr;
    if (stopIfCancelled()) {
      return;
    }
    if (status != QProcess::NormalExit || exitCode != 0) {
      fail(QStringLiteral("Extracting %1 failed: %2").arg(package.label, errorText->trimmed().isEmpty()
        ? QStringLiteral("tar exited with code %1").arg(exitCode) : errorText->trimmed()));
      return;
    }
    extract(index + 1);
  });
  connect(process, &QProcess::errorOccurred, this, [this, process, package](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart) {
      process->deleteLater();
      activeProcess = nullptr;
      fail(QStringLiteral("Extracting %1 failed: %2").arg(package.label, process->errorString()));
    }
  });
  process->start(tarPath(), {QStringLiteral("-xvf"), stagedZipPath(stagingRoot, package), QStringLiteral("-C"), folder});
}

void InstallJob::apply()
{
  isApplying = true;
  emit progressChanged(0, 0);
  if (installPlan.isUpdate()) {
    emit statusChanged(installPlan.isAttach ? QStringLiteral("Adding the tools...")
      : installPlan.isToolChange ? QStringLiteral("Applying the tool changes...") : QStringLiteral("Applying the update..."));
    applyUpdate();
  } else {
    emit statusChanged(QStringLiteral("Moving the files into place..."));
    applyInstall();
  }
}

void InstallJob::applyInstall()
{
  if (!QDir().mkpath(QFileInfo(installPlan.folder).absolutePath())) {
    fail(QStringLiteral("Could not create %1.").arg(QFileInfo(installPlan.folder).absolutePath()));
    return;
  }
  if (!QDir().rename(extractFolder(installPlan.client), installPlan.folder)) {
    fail(QStringLiteral("Could not move the client into %1.").arg(installPlan.folder));
    return;
  }
  for (const Package& tool : installPlan.tools) {
    const QDir source(extractFolder(tool));
    QDirIterator files(source.absolutePath(), QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (files.hasNext()) {
      const QString from = files.next();
      const QString relative = source.relativeFilePath(from);
      const QString to = QDir(installPlan.folder).filePath(relative);
      if (QFile::exists(to)) {
        QDir(installPlan.folder).removeRecursively();
        fail(QStringLiteral("%1 and the client both ship %2; the install was undone.").arg(tool.label, relative));
        return;
      }
      if (!QDir().mkpath(QFileInfo(to).absolutePath()) || !QFile::rename(from, to)) {
        QDir(installPlan.folder).removeRecursively();
        fail(QStringLiteral("Could not move %1 of %2 into place; the install was undone.").arg(relative, tool.label));
        return;
      }
    }
  }
  Receipt receipt{installPlan.folder, {installPlan.client.name, installPlan.client.latest->version, installPlan.client.latest->sha256},
    {}, ownedFiles};
  for (const Package& tool : installPlan.tools) {
    receipt.tools.append({tool.name, tool.latest->version, tool.latest->sha256});
  }
  ApplySummary summary;
  summary.written = static_cast<int>(ownedFiles.size());
  complete(receipt, summary);
}

// The rule Steam applies: a file whose package entry did not change is left alone whatever its state, a changed
// entry is written over, a dropped entry is deleted, and anything no package lists is never touched. A write-once
// file that exists is never written over.
void InstallJob::applyUpdate()
{
  const Receipt& old = *installPlan.existing;
  const QDir folder(installPlan.folder);
  QSet<QString> fetchedNames;
  for (const Package& package : queue) {
    fetchedNames.insert(package.name);
  }
  const QSet<QString> removedNames(installPlan.removed.cbegin(), installPlan.removed.cend());
  QMap<QString, OwnedFile> files;
  QStringList toDelete;
  ApplySummary summary;
  for (auto entry = old.files.constBegin(); entry != old.files.constEnd(); ++entry) {
    const QString& owner = entry.value().package;
    if (removedNames.contains(owner) && entry.value().isWriteOnce) {
      if (QFile::exists(folder.filePath(entry.key()))) {
        summary.kept.append(entry.key());
      }
    } else if (removedNames.contains(owner) || (fetchedNames.contains(owner) && !ownedFiles.contains(entry.key()))) {
      toDelete.append(entry.key());
    } else if (!fetchedNames.contains(owner)) {
      files.insert(entry.key(), entry.value());
    }
  }
  const qint64 total = toDelete.size() + ownedFiles.size();
  qint64 done = 0;
  for (const QString& path : toDelete) {
    const QString target = folder.filePath(path);
    if (QFile::exists(target)) {
      if (!QFile::remove(target)) {
        fail(QStringLiteral("Could not remove %1; the update is incomplete. Press Update again to finish it.").arg(path));
        return;
      }
      ++summary.deleted;
    }
    emit progressChanged(++done, total);
  }
  for (auto entry = ownedFiles.constBegin(); entry != ownedFiles.constEnd(); ++entry) {
    const QString& path = entry.key();
    const OwnedFile& file = entry.value();
    files.insert(path, file);
    emit progressChanged(++done, total);
    const auto previous = old.files.constFind(path);
    if (previous != old.files.constEnd() && previous->package == file.package && previous->byteSize == file.byteSize
        && previous->crc32 == file.crc32) {
      continue;
    }
    const QString target = folder.filePath(path);
    if (file.isWriteOnce && QFile::exists(target)) {
      summary.kept.append(path);
      continue;
    }
    if (QFile::exists(target)) {
      if (!QFile::remove(target)) {
        fail(QStringLiteral("Could not replace %1 (is the game running?); the change is incomplete. Do it again to finish it.").arg(path));
        return;
      }
      if (previous == old.files.constEnd()) {
        summary.replaced.append(path);
      }
    }
    const QString source = QDir(extractFolder(fetchedPackage(file.package))).filePath(path);
    if (!QDir().mkpath(QFileInfo(target).absolutePath()) || !QFile::rename(source, target)) {
      fail(QStringLiteral("Could not write %1; the update is incomplete. Press Update again to finish it.").arg(path));
      return;
    }
    ++summary.written;
  }
  Receipt receipt = old;
  receipt.tools.clear();
  receipt.files = files;
  if (fetchedNames.contains(installPlan.client.name)) {
    receipt.client = {installPlan.client.name, installPlan.client.latest->version, installPlan.client.latest->sha256};
  }
  for (const Package& tool : installPlan.tools) {
    if (fetchedNames.contains(tool.name)) {
      receipt.tools.append({tool.name, tool.latest->version, tool.latest->sha256});
      continue;
    }
    bool isFound = false;
    for (const InstalledPackage& installed : old.tools) {
      if (installed.name == tool.name) {
        receipt.tools.append(installed);
        isFound = true;
      }
    }
    if (!isFound) {
      fatal(QStringLiteral("The update plan lists %1, which is neither fetched nor installed.").arg(tool.name));
    }
  }
  complete(receipt, summary);
}

void InstallJob::complete(Receipt receipt, const ApplySummary& summary)
{
  removeExtractFolders();
  const QString problem = writeReceipt(receipt);
  if (!problem.isEmpty()) {
    fail(installPlan.isUpdate()
      ? QStringLiteral("%1 The files are updated but Ankh could not record it; press Update again.").arg(problem)
      : QStringLiteral("%1 The files are in %2 but Ankh has no record of them; remove that folder and install again.")
          .arg(problem, installPlan.folder));
    return;
  }
  for (const Package& package : queue) {
    QFile::remove(stagedZipPath(stagingRoot, package));
  }
  emit finished(receipt, summary);
}
