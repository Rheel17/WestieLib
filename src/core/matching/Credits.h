#pragma once

#include <span>
#include <string>
#include <vector>

#include "core/matching/TitleParts.h"

namespace westie::core {

std::vector<std::vector<std::string>> name_pieces(std::span<const std::string> name);
std::vector<std::vector<std::string>> credited_names(const TitleParts& title);
bool is_credit(std::span<const std::string> part, const std::vector<std::vector<std::string>>& known);
bool is_credited_lead(std::span<const std::string> lead, const std::vector<std::vector<std::string>>& known);

}
