#include "platform/win32/LocalTime.h"

#include <cstdint>
#include <ratio>

#include <windows.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

using FileTimeTicks = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;

// FILETIME counts from 1601, the system clock from 1970
constexpr std::chrono::seconds FileTimeEpochOffset{11'644'473'600};

}

std::chrono::local_seconds to_local_time(std::chrono::system_clock::time_point time) {
    ULARGE_INTEGER ticks{};
    // In ticks first: the offset in the system clock's nanoseconds doesn't fit 64 bits
    ticks.QuadPart = (std::chrono::duration_cast<FileTimeTicks>(time.time_since_epoch()) + FileTimeEpochOffset).count();
    FILETIME utc{.dwLowDateTime = ticks.LowPart, .dwHighDateTime = ticks.HighPart};

    SYSTEMTIME utc_time;
    SYSTEMTIME local;
    check(FileTimeToSystemTime(&utc, &utc_time), "FileTimeToSystemTime");
    check(SystemTimeToTzSpecificLocalTime(nullptr, &utc_time, &local), "SystemTimeToTzSpecificLocalTime");

    std::chrono::year_month_day day{std::chrono::year(local.wYear), std::chrono::month(local.wMonth), std::chrono::day(local.wDay)};
    return std::chrono::local_days(day) + std::chrono::hours(local.wHour) + std::chrono::minutes(local.wMinute) + std::chrono::seconds(local.wSecond);
}

}
