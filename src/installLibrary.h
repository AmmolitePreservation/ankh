#pragma once

#include "ammoliteClient.h"

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <optional>

struct InstalledPackage {
  QString name;
  QString version;
  QString sha256;
};

struct OwnedFile {
  QString package;
  qint64 byteSize;
  quint32 crc32;
  bool isWriteOnce;
};

struct Receipt {
  QString folder;
  InstalledPackage client;
  QList<InstalledPackage> plugins;
  QMap<QString, OwnedFile> files;
  // Set for plugins attached to the player's own copy of a game: there is no client package, and the folder is theirs.
  QString attachedGame;
  QString attachedVariant;

  bool isAttached() const { return !attachedGame.isEmpty(); }
};

struct LibraryScan {
  QList<Receipt> receipts;
  QStringList problems;
};

QString defaultLibraryRoot();
QString libraryRootProblem(const QString& root);
QString installFolderNameFor(const Package& client);
QString receiptPath(const QString& folder);
QString writeReceipt(const Receipt& receipt);
std::optional<Receipt> readReceipt(const QString& folder, QString& problem);
LibraryScan scanLibrary(const QString& root, const QStringList& rememberedFolders);
const Receipt* receiptForClient(const QList<Receipt>& receipts, const QString& clientName);
const Receipt* receiptForAttached(const QList<Receipt>& receipts, const QString& game, const QString& variant);
