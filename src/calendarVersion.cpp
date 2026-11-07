#include "calendarVersion.h"

#include <QRegularExpression>

std::optional<CalendarVersion> parseCalendarVersion(const QString& text)
{
  static const QRegularExpression pattern(QStringLiteral("^v?(\\d{4})\\.(\\d{2})\\.(\\d{2})\\.(\\d{4})$"));
  const QRegularExpressionMatch match = pattern.match(text);
  if (!match.hasMatch()) {
    return std::nullopt;
  }
  return CalendarVersion{match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt(), match.captured(4).toInt()};
}
