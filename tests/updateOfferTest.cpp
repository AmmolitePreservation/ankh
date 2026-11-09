#include "updateOffer.h"

#include <QtTest>

namespace {

ReleaseNote note(const QString& version, const QString& notes)
{
  return {version, notes, QStringLiteral("2026-11-%1T05:18:56Z").arg(version.mid(8, 2))};
}

AnkhRelease releaseTagged(const QString& version, const QList<ReleaseNote>& history)
{
  return {version, QStringLiteral("v%1").arg(version), history.isEmpty() ? QString() : history.first().notes,
    QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/tag/v%1").arg(version)),
    QStringLiteral("2026-11-08T05:18:56Z"),
    {{QStringLiteral("manifest.json"), QUrl(QStringLiteral("https://example.test/%1/manifest.json").arg(version))}}, history};
}

const QList<ReleaseNote> fourReleases = {
  note(QStringLiteral("2026.11.09.0141"), QStringLiteral("- Choose a folder for one install")),
  note(QStringLiteral("2026.11.09.0123"), QStringLiteral("- Refuse an unrooted library folder\n")),
  note(QStringLiteral("2026.11.09.0012"), QString()),
  note(QStringLiteral("2026.11.08.2207"), QStringLiteral("- Install games")),
};

}

class UpdateOfferTest : public QObject {
  Q_OBJECT

private slots:
  void theOfferInstallsTheNewestReleaseWithItsPageAndAssets()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), fourReleases), QStringLiteral("2026.11.09.0123"));
    QVERIFY(check.problem.isEmpty());
    QVERIFY(check.update.has_value());
    QCOMPARE(check.update->version, QStringLiteral("2026.11.09.0141"));
    QCOMPARE(check.update->notes, QStringLiteral("2026.11.09.0141 (2026-11-09)\n- Choose a folder for one install"));
    QCOMPARE(check.update->page, QUrl(QStringLiteral("https://github.com/AmmolitePreservation/ankh/releases/tag/v2026.11.09.0141")));
    QCOMPARE(check.update->assets.value(QStringLiteral("manifest.json")), QUrl(QStringLiteral("https://example.test/2026.11.09.0141/manifest.json")));
  }

  void theNotesCoverEveryReleaseNewerThanTheRunningOneNewestFirst()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), fourReleases), QStringLiteral("2026.11.08.2207"));
    QVERIFY(check.update.has_value());
    QCOMPARE(check.update->version, QStringLiteral("2026.11.09.0141"));
    QCOMPARE(check.update->notes, QStringLiteral(
      "2026.11.09.0141 (2026-11-09)\n- Choose a folder for one install\n\n"
      "2026.11.09.0123 (2026-11-09)\n- Refuse an unrooted library folder\n\n"
      "2026.11.09.0012 (2026-11-09)\nNo release notes."));
  }

  void theNotesAreOrderedByVersionWhateverOrderTheListCameIn()
  {
    QList<ReleaseNote> shuffled = {fourReleases.at(2), fourReleases.at(0), fourReleases.at(3), fourReleases.at(1)};
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), shuffled), QStringLiteral("2026.11.09.0012"));
    QVERIFY(check.update.has_value());
    QCOMPARE(check.update->notes, QStringLiteral(
      "2026.11.09.0141 (2026-11-09)\n- Choose a folder for one install\n\n"
      "2026.11.09.0123 (2026-11-09)\n- Refuse an unrooted library folder"));
  }

  void aRunningVersionOlderThanTheWholeListIsToldTheListStopsThere()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), fourReleases.mid(0, 2)), QStringLiteral("2026.10.01.0900"));
    QVERIFY(check.update.has_value());
    QVERIFY2(check.update->notes.endsWith(QStringLiteral("\n\nReleases before 2026.11.09.0123 are not listed.")), qPrintable(check.update->notes));
  }

  void theSameOrAnOlderReleaseIsNotAnUpdate()
  {
    for (const char* version : {"2026.11.09.0141", "2026.11.09.0200", "2027.01.01.0000"}) {
      const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), fourReleases), QLatin1String(version));
      QVERIFY2(!check.update.has_value(), version);
      QVERIFY2(check.problem.isEmpty(), version);
    }
  }

  void aTagThatIsNotACalendarVersionIsAProblemNotAnUpdate()
  {
    const ReleaseCheck check = checkRelease(releaseTagged(QStringLiteral("1.2.3"), fourReleases), QStringLiteral("2026.11.08.0514"));
    QVERIFY(!check.update.has_value());
    QCOMPARE(check.problem, QStringLiteral("The latest release tag is not a calendar version: v1.2.3"));
  }

  void aReleaseListThatDoesNotLeadWithTheLatestIsAProblem()
  {
    const ReleaseCheck missing = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), fourReleases.mid(1)), QStringLiteral("2026.11.08.2207"));
    QVERIFY(!missing.update.has_value());
    QCOMPARE(missing.problem, QStringLiteral("The release list does not start with the latest release 2026.11.09.0141."));
    const ReleaseCheck empty = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), {}), QStringLiteral("2026.11.08.2207"));
    QCOMPARE(empty.problem, QStringLiteral("The release list does not start with the latest release 2026.11.09.0141."));
    QList<ReleaseNote> odd = fourReleases;
    odd.append(note(QStringLiteral("nightly"), QStringLiteral("x")));
    const ReleaseCheck unparsed = checkRelease(releaseTagged(QStringLiteral("2026.11.09.0141"), odd), QStringLiteral("2026.11.08.2207"));
    QCOMPARE(unparsed.problem, QStringLiteral("The release list has a version that is not a calendar version: nightly"));
  }
};

QTEST_GUILESS_MAIN(UpdateOfferTest)
#include "updateOfferTest.moc"
