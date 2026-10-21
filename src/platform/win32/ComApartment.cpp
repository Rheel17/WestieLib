#include "platform/win32/ComApartment.h"

#include <windows.h>
#include <objbase.h>

#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

ComApartment::ComApartment() {
    check_hresult(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE), "CoInitializeEx");
}

ComApartment::~ComApartment() {
    CoUninitialize();
}

}
