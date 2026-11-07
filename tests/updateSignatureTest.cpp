#include "updateSignature.h"

#include <QFile>
#include <QtTest>

namespace {

QByteArray fixture(const QString& name)
{
  QFile file(QStringLiteral(ANKH_FIXTURE_DIR "/") + name);
  if (!file.open(QIODevice::ReadOnly)) {
    qFatal("Missing fixture %s", qPrintable(name));
  }
  return file.readAll();
}

}

class UpdateSignatureTest : public QObject {
  Q_OBJECT

private slots:
  void acceptsAMessageSignedByTheKey()
  {
    QVERIFY(verifyWithPublicKey(QByteArray::fromBase64(fixture("signerPublic.b64")), fixture("message.json"), QByteArray::fromBase64(fixture("message.json.sig"))));
  }

  void rejectsAnAlteredMessage()
  {
    QByteArray message = fixture("message.json");
    message[10] = message.at(10) ^ 0x01;
    QVERIFY(!verifyWithPublicKey(QByteArray::fromBase64(fixture("signerPublic.b64")), message, QByteArray::fromBase64(fixture("message.json.sig"))));
  }

  void rejectsAnAlteredSignature()
  {
    QByteArray signature = QByteArray::fromBase64(fixture("message.json.sig"));
    signature[5] = signature.at(5) ^ 0x01;
    QVERIFY(!verifyWithPublicKey(QByteArray::fromBase64(fixture("signerPublic.b64")), fixture("message.json"), signature));
  }

  void rejectsASignatureCheckedAgainstAnotherKey()
  {
    QVERIFY(!verifyWithPublicKey(QByteArray::fromBase64(fixture("otherPublic.b64")), fixture("message.json"), QByteArray::fromBase64(fixture("message.json.sig"))));
  }

  void rejectsASignatureOfTheWrongLength()
  {
    QByteArray signature = QByteArray::fromBase64(fixture("message.json.sig"));
    signature.chop(1);
    QVERIFY(!verifyWithPublicKey(QByteArray::fromBase64(fixture("signerPublic.b64")), fixture("message.json"), signature));
  }
};

QTEST_MAIN(UpdateSignatureTest)
#include "updateSignatureTest.moc"
