#pragma once

#include <QString>
#include <array>
#include <optional>

using CalendarVersion = std::array<int, 4>;

std::optional<CalendarVersion> parseCalendarVersion(const QString& text);
