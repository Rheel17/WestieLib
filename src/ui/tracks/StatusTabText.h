#pragma once

#include <string>

#include "ui/tracks/StatusCounts.h"

namespace westie::ui {

const wchar_t* status_word(StatusTab tab);
int status_count(const StatusCounts& counts, StatusTab tab);
std::wstring status_tab_text(const StatusCounts& counts, StatusTab tab);

}
