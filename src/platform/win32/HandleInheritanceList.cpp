#include "platform/win32/HandleInheritanceList.h"

#include <utility>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr DWORD AttributeCount = 1;

}

HandleInheritanceList::HandleInheritanceList(std::vector<HANDLE> handles) : handles_(std::move(handles)) {
    SIZE_T size = 0;
    // Asked without a list, it only reports the size it needs, and fails for that reason
    InitializeProcThreadAttributeList(nullptr, AttributeCount, 0, &size);
    buffer_.resize(size);
    check(InitializeProcThreadAttributeList(get(), AttributeCount, 0, &size), "InitializeProcThreadAttributeList");

    if (!UpdateProcThreadAttribute(get(), 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles_.data(), handles_.size() * sizeof(HANDLE), nullptr, nullptr)) {
        // The destructor doesn't run for a constructor that throws
        DWORD error = GetLastError();
        DeleteProcThreadAttributeList(get());
        throw_error(error, "UpdateProcThreadAttribute");
    }
}

HandleInheritanceList::~HandleInheritanceList() {
    DeleteProcThreadAttributeList(get());
}

LPPROC_THREAD_ATTRIBUTE_LIST HandleInheritanceList::get() noexcept {
    return reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(buffer_.data());
}

}
