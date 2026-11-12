#pragma once

#include "platform/TextNormalizer.h"

namespace westie::tests {

void check_match_words(const platform::TextNormalizer& normalizer);
void check_title_parts(const platform::TextNormalizer& normalizer);
void check_names(const platform::TextNormalizer& normalizer);
void check_judgements(const platform::TextNormalizer& normalizer);
void check_log_texts();

}
