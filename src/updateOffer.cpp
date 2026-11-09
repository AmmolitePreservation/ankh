#include "updateOffer.h"

#include "calendarVersion.h"
#include "fatal.h"

#include <QStringList>
#include <algorithm>

namespace {

struct ListedNote {
  CalendarVersion version;
  ReleaseNote note;
};

}

// The offer installs the newest release; its notes are those of every release newer than the running one, newest
// first, because updating skips straight past the versions in between.
ReleaseCheck checkRelease(const AnkhRelease& release, const QString& currentVersion)
{
  const auto current = parseCalendarVersion(currentVersion);
  if (!current) {
    fatal(QStringLiteral("The running version is not a calendar version: %1").arg(currentVersion));
  }
  const auto offered = parseCalendarVersion(release.version);
  if (!offered) {
    return {std::nullopt, QStringLiteral("The latest release tag is not a calendar version: %1").arg(release.tag)};
  }
  if (*offered <= *current) {
    return {std::nullopt, QString()};
  }
  QList<ListedNote> newer;
  bool reachesCurrent = false;
  for (const ReleaseNote& note : release.history) {
    const auto listed = parseCalendarVersion(note.version);
    if (!listed) {
      return {std::nullopt, QStringLiteral("The release list has a version that is not a calendar version: %1").arg(note.version)};
    }
    if (*listed <= *current) {
      reachesCurrent = true;
    } else {
      newer.append({*listed, note});
    }
  }
  std::sort(newer.begin(), newer.end(), [](const ListedNote& left, const ListedNote& right) { return right.version < left.version; });
  if (newer.isEmpty() || newer.first().version != *offered) {
    return {std::nullopt, QStringLiteral("The release list does not start with the latest release %1.").arg(release.version)};
  }
  QStringList sections;
  for (const ListedNote& listed : newer) {
    const QString text = listed.note.notes.trimmed();
    sections.append(QStringLiteral("%1 (%2)\n%3").arg(listed.note.version, listed.note.publishedAt.left(10),
      text.isEmpty() ? QStringLiteral("No release notes.") : text));
  }
  if (!reachesCurrent) {
    sections.append(QStringLiteral("Releases before %1 are not listed.").arg(newer.last().note.version));
  }
  return {UpdateOffer{release.version, sections.join(QStringLiteral("\n\n")), release.page, release.assets}, QString()};
}
