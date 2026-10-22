#pragma once

#include <cstddef>
#include <vector>

#include <windows.h>

namespace westie::platform::win32 {

class HandleInheritanceList {
public:
    explicit HandleInheritanceList(std::vector<HANDLE> handles);
    ~HandleInheritanceList();

    HandleInheritanceList(const HandleInheritanceList&) = delete;
    HandleInheritanceList& operator=(const HandleInheritanceList&) = delete;

    LPPROC_THREAD_ATTRIBUTE_LIST get() noexcept;

private:
    // The attribute list points into it
    std::vector<HANDLE> handles_;
    std::vector<std::byte> buffer_;
};

}
