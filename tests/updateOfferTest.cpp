#include "updateOffer.h"

#include <QtTest>

namespace {

AnkhRelease releaseTagged(const QString& version)
{
  return {version, QStringLiteral("v%1").arg(version), QStringLiteral("- Library toggles"),
    QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/tag/v%1").arg(version)),
    QStringLiteral("2026-11-08T05:18:56Z"),
    {{QStringLiteral("manifest.json"), QUrl(QStringLiteral("https://example.test/%1/manifest.json").arg(version))}}};
}

}

class UpdateOfferTest : public QObject {
  Q_OBJECT

private slots:
  void aNewerReleaseIsAnUpdateWithItsNotesPageAndAssets()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0100")), QStringLiteral("2026.11.08.0514"));
    QVERIFY(check.problem.isEmpty());
    QVERIFY(check.update.has_value());
    QCOMPARE(check.update->version, QStringLiteral("2026.11.09.0100"));
    QCOMPARE(check.update->notes, QStringLiteral("- Library toggles"));
    QCOMPARE(check.update->page, QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/tag/v2026.11.09.0100")));
    QCOMPARE(check.update->assets.value(QStringLiteral("manifest.json")), QUrl(QStringLiteral("https://example.test/2026.11.09.0100/manifest.json")));
  }

  void theSameOrAnOlderReleaseIsNotAnUpdate()
  {
    for (const char* version : {"2026.11.08.0514", "2026.11.08.0334", "2025.12.31.2359"}) {
      const ReleaseCheck check = checkRelease(releaseTagged(QLatin1String(version)), QStringLiteral("2026.11.08.0514"));
      QVERIFY2(!check.update.has_value(), version);
      QVERIFY2(check.problem.isEmpty(), version);
    }
  }

  void aTagThatIsNotACalendarVersionIsAProblemNotAnUpdate()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("1.2.3")), QStringLiteral("2026.11.08.0514"));
    QVERIFY(!check.update.has_value());
    QCOMPARE(check.problem, QStringLiteral("The latest release tag is not a calendar version: v1.2.3"));
  }
};

QTEST_GUILESS_MAIN(UpdateOfferTest)
#include "updateOfferTest.moc"
