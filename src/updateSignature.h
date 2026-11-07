#pragma once

#include <QByteArray>

bool verifyWithPublicKey(const QByteArray& publicPoint, const QByteArray& message, const QByteArray& signature);
bool verifyManifestSignature(const QByteArray& manifest, const QByteArray& signatureBase64);
