#include "platform/win32/Win32TimeZone.h"

#include "platform/win32/LocalTime.h"

namespace westie::platform::win32 {

std::chrono::local_time<std::chrono::milliseconds> Win32TimeZone::to_local(std::chrono::system_clock::time_point time) const {
    return to_local_milliseconds(time);
}

}
