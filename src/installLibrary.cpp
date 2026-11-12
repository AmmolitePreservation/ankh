#include "installLibrary.h"

#include "installInfo.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>

namespace {

const QString receiptFolderName = QStringLiteral(".ankh");
const QString receiptFileName = QStringLiteral("receipt.json");

struct ReceiptError {
  QString message;
};

QJsonValue required(const QJsonObject& object, const char* key)
{
  const QJsonValue value = object.value(QLatin1String(key));
  if (value.isUndefined()) {
    throw ReceiptError{QStringLiteral("the field \"%1\" is missing").arg(QLatin1String(key))};
  }
  return value;
}

QString requiredText(const QJsonObject& object, const char* key)
{
  const QJsonValue value = required(object, key);
  if (!value.isString() || value.toString().isEmpty()) {
    throw ReceiptError{QStringLiteral("the field \"%1\" is not text").arg(QLatin1String(key))};
  }
  return value.toString();
}

qint64 requiredCount(const QJsonObject& object, const char* key)
{
  const QJsonValue value = required(object, key);
  if (!value.isDouble() || value.toDouble() != value.toInteger() || value.toInteger() < 0) {
    throw ReceiptError{QStringLiteral("the field \"%1\" is not a whole number").arg(QLatin1String(key))};
  }
  return value.toInteger();
}

bool requiredFlag(const QJsonObject& object, const char* key)
{
  const QJsonValue value = required(object, key);
  if (!value.isBool()) {
    throw ReceiptError{QStringLiteral("the field \"%1\" is not true or false").arg(QLatin1String(key))};
  }
  return value.toBool();
}

InstalledPackage packageFrom(const QJsonValue& value)
{
  if (!value.isObject()) {
    throw ReceiptError{QStringLiteral("a package entry is not an object")};
  }
  const QJsonObject object = value.toObject();
  return {requiredText(object, "name"), requiredText(object, "version"), requiredText(object, "sha256")};
}

QJsonObject packageJson(const InstalledPackage& package)
{
  return {{QStringLiteral("name"), package.name}, {QStringLiteral("version"), package.version},
    {QStringLiteral("sha256"), package.sha256}};
}

}

QString defaultLibraryRoot()
{
  return QDir::toNativeSeparators(QDir(installFolder()).filePath(QStringLiteral("clients")));
}

// A drive-relative "C:folder" or a bare name would resolve against the working directory, so only a rooted path
// is a library.
QString libraryRootProblem(const QString& root)
{
  static const QRegularExpression rooted(QStringLiteral("^(?:[A-Za-z]:[\\\\/]|\\\\\\\\[^\\\\/]+[\\\\/][^\\\\/]+)"));
  if (root.trimmed().isEmpty()) {
    return QStringLiteral("The library folder setting is empty.");
  }
  if (!rooted.match(root).hasMatch()) {
    return QStringLiteral("The library folder %1 is not an absolute path such as C:\\Games\\Ankh. Fix library/root in the "
      "settings file or remove it to use the default.").arg(root);
  }
  return QString();
}

QString installFolderNameFor(const Package& client)
{
  if (client.editions.isEmpty()) {
    return QString();
  }
  QString name = client.editions.first().game;
  if (client.editions.size() == 1) {
    name += QStringLiteral(" - ") + client.editions.first().label;
  }
  static const QString forbidden = QStringLiteral("\\/:*?\"<>|");
  for (QChar& letter : name) {
    if (forbidden.contains(letter) || letter.unicode() < 0x20) {
      letter = QLatin1Char('-');
    }
  }
  while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' '))) {
    name.chop(1);
  }
  return name;
}

QString receiptPath(const QString& folder)
{
  return QDir(folder).filePath(receiptFolderName + QLatin1Char('/') + receiptFileName);
}

QString writeReceipt(const Receipt& receipt)
{
  QJsonObject files;
  for (auto entry = receipt.files.constBegin(); entry != receipt.files.constEnd(); ++entry) {
    files.insert(entry.key(), QJsonObject{{QStringLiteral("package"), entry.value().package},
      {QStringLiteral("byteSize"), entry.value().byteSize}, {QStringLiteral("crc32"), static_cast<qint64>(entry.value().crc32)},
      {QStringLiteral("writeOnce"), entry.value().isWriteOnce}});
  }
  QJsonArray plugins;
  for (const InstalledPackage& plugin : receipt.plugins) {
    plugins.append(packageJson(plugin));
  }
  QJsonObject object{{QStringLiteral("plugins"), plugins}, {QStringLiteral("files"), files}};
  if (receipt.isAttached()) {
    object.insert(QStringLiteral("client"), QJsonValue::Null);
    object.insert(QStringLiteral("attached"), QJsonObject{{QStringLiteral("game"), receipt.attachedGame},
      {QStringLiteral("variant"), receipt.attachedVariant}});
  } else {
    object.insert(QStringLiteral("client"), packageJson(receipt.client));
  }
  const QString path = receiptPath(receipt.folder);
  if (!QDir().mkpath(QFileInfo(path).absolutePath())) {
    return QStringLiteral("Could not create %1.").arg(QDir::toNativeSeparators(QFileInfo(path).absolutePath()));
  }
  QSaveFile file(path);
  if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(object).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
    return QStringLiteral("Could not write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString());
  }
  return QString();
}

std::optional<Receipt> readReceipt(const QString& folder, QString& problem)
{
  const QString path = receiptPath(folder);
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    problem = QStringLiteral("%1 could not be read: %2").arg(QDir::toNativeSeparators(path), file.errorString());
    return std::nullopt;
  }
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
  if (!document.isObject()) {
    problem = QStringLiteral("%1 is not a JSON object: %2").arg(QDir::toNativeSeparators(path), parseError.errorString());
    return std::nullopt;
  }
  try {
    const QJsonObject object = document.object();
    Receipt receipt{QDir::toNativeSeparators(QDir(folder).absolutePath()), {}, {}, {}};
    if (required(object, "client").isNull()) {
      const QJsonValue attached = required(object, "attached");
      if (!attached.isObject()) {
        throw ReceiptError{QStringLiteral("the field \"attached\" is not an object")};
      }
      receipt.attachedGame = requiredText(attached.toObject(), "game");
      receipt.attachedVariant = requiredText(attached.toObject(), "variant");
    } else {
      receipt.client = packageFrom(required(object, "client"));
    }
    // Releases before 2026.11.12 wrote this list as "tools". Such a receipt is read as it stands and written back
    // below with the field's name of today, so the install it records stays installed.
    const bool isEarlierReceipt = !object.contains(QLatin1String("plugins")) && object.contains(QLatin1String("tools"));
    const QJsonValue plugins = required(object, isEarlierReceipt ? "tools" : "plugins");
    if (!plugins.isArray()) {
      throw ReceiptError{QStringLiteral("the field \"plugins\" is not a list")};
    }
    for (const QJsonValue& plugin : plugins.toArray()) {
      receipt.plugins.append(packageFrom(plugin));
    }
    const QJsonValue files = required(object, "files");
    if (!files.isObject()) {
      throw ReceiptError{QStringLiteral("the field \"files\" is not an object")};
    }
    const QJsonObject fileObject = files.toObject();
    for (auto entry = fileObject.constBegin(); entry != fileObject.constEnd(); ++entry) {
      if (!entry.value().isObject()) {
        throw ReceiptError{QStringLiteral("the file entry %1 is not an object").arg(entry.key())};
      }
      const QJsonObject owned = entry.value().toObject();
      receipt.files.insert(entry.key(), {requiredText(owned, "package"), requiredCount(owned, "byteSize"),
        static_cast<quint32>(requiredCount(owned, "crc32")), requiredFlag(owned, "writeOnce")});
    }
    if (isEarlierReceipt) {
      file.close();
      const QString writeProblem = writeReceipt(receipt);
      if (!writeProblem.isEmpty()) {
        throw ReceiptError{QStringLiteral("it is from an earlier release and could not be rewritten (%1)").arg(writeProblem)};
      }
    }
    return receipt;
  } catch (const ReceiptError& error) {
    problem = QStringLiteral("%1 is not a valid install record: %2.").arg(QDir::toNativeSeparators(path), error.message);
    return std::nullopt;
  }
}

// Installs are the library's subfolders that carry a receipt, plus the folders remembered from installs made
// elsewhere or under an earlier library folder.
LibraryScan scanLibrary(const QString& root, const QStringList& rememberedFolders)
{
  LibraryScan scan;
  QStringList folders;
  const QDir rootDir(root);
  if (rootDir.exists()) {
    for (const QFileInfo& entry : rootDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
      if (QFile::exists(receiptPath(entry.absoluteFilePath()))) {
        folders.append(entry.absoluteFilePath());
      }
    }
  }
  folders.append(rememberedFolders);
  QSet<QString> seen;
  for (const QString& folder : folders) {
    const QString key = QDir::cleanPath(QDir(folder).absolutePath()).toLower();
    if (seen.contains(key)) {
      continue;
    }
    seen.insert(key);
    QString problem;
    const std::optional<Receipt> receipt = readReceipt(folder, problem);
    if (receipt) {
      scan.receipts.append(*receipt);
    } else {
      scan.problems.append(problem);
    }
  }
  return scan;
}

const Receipt* receiptForAttached(const QList<Receipt>& receipts, const QString& game, const QString& variant)
{
  for (const Receipt& receipt : receipts) {
    if (receipt.isAttached() && receipt.attachedGame == game && receipt.attachedVariant == variant) {
      return &receipt;
    }
  }
  return nullptr;
}

const Receipt* receiptForClient(const QList<Receipt>& receipts, const QString& clientName)
{
  for (const Receipt& receipt : receipts) {
    if (!receipt.isAttached() && receipt.client.name == clientName) {
      return &receipt;
    }
  }
  return nullptr;
}
