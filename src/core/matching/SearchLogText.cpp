#include "core/matching/SearchLogText.h"

#include <format>

#include "core/matching/MatchText.h"

namespace westie::core {

std::string search_started_text(std::string_view query) {
    return std::format("Searching YouTube Music for \"{}\"", query);
}

std::string search_finished_text(std::string_view query, std::size_t results) {
    if (results == 0) {
        return std::format("Searched \"{}\": no results", query);
    }
    return std::format("Searched \"{}\": {} result{}", query, results, results == 1 ? "" : "s");
}

std::string best_result_text(std::string_view query, const std::string& title, const std::vector<std::string>& artists, double confidence) {
    return std::format("Best result for \"{}\": {}", query, result_text(title, artists, confidence));
}

std::string search_failed_text(std::string_view query, std::string_view error, bool refused) {
    if (refused) {
        return std::format("YouTube Music refused the search for \"{}\": {}", query, error);
    }
    return std::format("Search for \"{}\" failed: {}", query, error);
}

}
