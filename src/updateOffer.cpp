#include "updateOffer.h"

#include "calendarVersion.h"
#include "fatal.h"

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
  return {UpdateOffer{release.version, release.notes, release.page, release.assets}, QString()};
}
