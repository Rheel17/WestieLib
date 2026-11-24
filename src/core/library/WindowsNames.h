#pragma once

#include <string_view>

namespace westie::core {

enum class NameProblem { None, EndsInSpaceOrPeriod, ReservedName };

NameProblem windows_name_problem(std::string_view name);

}
