#pragma once

#include <string>
#include <vector>

#include "core/state/Match.h"
#include "core/state/MatchDoubt.h"

namespace westie::core {

std::string result_text(const std::string& title, const std::vector<std::string>& artists, double confidence);
std::string match_text(const Match& match);
std::string doubts_text(const std::vector<MatchDoubt>& doubts);

}
