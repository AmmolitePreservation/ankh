#include "updateSignature.h"

#include "fatal.h"

#include <QCryptographicHash>
#include <QFile>

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

namespace {

constexpr int pointLength = 64;
constexpr int signatureLength = 64;
constexpr int hashLength = 32;

QList<QByteArray> acceptedKeys()
{
  QFile file(QStringLiteral(":/updatePublicKeys.txt"));
  if (!file.open(QIODevice::ReadOnly)) {
    fatal(QStringLiteral("The update public keys are missing from this build."));
  }
  QList<QByteArray> keys;
  for (const QByteArray& line : file.readAll().split('\n')) {
    const QByteArray trimmed = line.trimmed();
    if (trimmed.isEmpty()) {
      continue;
    }
    const QByteArray point = QByteArray::fromBase64(trimmed);
    if (point.size() != pointLength) {
      fatal(QStringLiteral("An update public key in this build is not a 64-byte point."));
    }
    keys.append(point);
  }
  if (keys.isEmpty()) {
    fatal(QStringLiteral("This build lists no update public keys."));
  }
  return keys;
}

}

bool verifyWithPublicKey(const QByteArray& publicPoint, const QByteArray& message, const QByteArray& signature)
{
  if (publicPoint.size() != pointLength) {
    fatal(QStringLiteral("A public key must be a 64-byte point."));
  }
  if (signature.size() != signatureLength) {
    return false;
  }
  const QByteArray digest = QCryptographicHash::hash(message, QCryptographicHash::Sha256);
  if (digest.size() != hashLength) {
    fatal(QStringLiteral("SHA-256 did not produce 32 bytes."));
  }

  BCRYPT_ALG_HANDLE algorithm = nullptr;
  if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_ECDSA_P256_ALGORITHM, nullptr, 0) < 0) {
    fatal(QStringLiteral("Windows could not open its ECDSA provider."));
  }
  QByteArray blob;
  BCRYPT_ECCKEY_BLOB header{BCRYPT_ECDSA_PUBLIC_P256_MAGIC, 32};
  blob.append(reinterpret_cast<const char*>(&header), sizeof(header));
  blob.append(publicPoint);
  BCRYPT_KEY_HANDLE key = nullptr;
  const NTSTATUS imported = BCryptImportKeyPair(algorithm, nullptr, BCRYPT_ECCPUBLIC_BLOB, &key,
    reinterpret_cast<PUCHAR>(blob.data()), static_cast<ULONG>(blob.size()), 0);
  if (imported < 0) {
    BCryptCloseAlgorithmProvider(algorithm, 0);
    fatal(QStringLiteral("Windows rejected an update public key."));
  }
  const NTSTATUS verified = BCryptVerifySignature(key, nullptr,
    reinterpret_cast<PUCHAR>(const_cast<char*>(digest.constData())), static_cast<ULONG>(digest.size()),
    reinterpret_cast<PUCHAR>(const_cast<char*>(signature.constData())), static_cast<ULONG>(signature.size()), 0);
  BCryptDestroyKey(key);
  BCryptCloseAlgorithmProvider(algorithm, 0);
  return verified >= 0;
}

bool verifyManifestSignature(const QByteArray& manifest, const QByteArray& signatureBase64)
{
  const QByteArray signature = QByteArray::fromBase64(signatureBase64.trimmed());
  for (const QByteArray& key : acceptedKeys()) {
    if (verifyWithPublicKey(key, manifest, signature)) {
      return true;
    }
  }
  return false;
}
