#pragma once

#include <string>

#include <windows.h>

#include "platform/win32/UniqueHandle.h"

namespace westie::platform::win32 {

struct Pipe {
    UniqueFile read{};
    UniqueFile write{};
};

Pipe create_output_pipe();
std::string read_all(HANDLE pipe);

}
