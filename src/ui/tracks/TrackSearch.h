#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/state/EffectiveMetadata.h"
#include "platform/TextNormalizer.h"
#include "ui/tracks/TrackRows.h"

namespace westie::ui {

std::string search_text(const platform::TextNormalizer& normalizer, const core::TrackMetadata& metadata);
std::vector<std::string> search_words(const platform::TextNormalizer& normalizer, std::wstring_view query);
std::vector<TrackRow> rows_matching(const std::vector<TrackRow>& rows, const std::vector<std::string>& words);

}
