#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "platform/TextNormalizer.h"

namespace westie::core {

std::string folded_text(const platform::TextNormalizer& normalizer, std::string_view text);
std::vector<std::string> words_of(std::string_view folded);
std::vector<std::string> match_words(const platform::TextNormalizer& normalizer, std::string_view text);
std::string letters_of(std::span<const std::string> words);

}
