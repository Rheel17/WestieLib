#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace westie::core {

std::string search_started_text(std::string_view query);
std::string search_finished_text(std::string_view query, std::size_t results);
std::string best_result_text(std::string_view query, const std::string& title, const std::vector<std::string>& artists, double confidence);
std::string search_failed_text(std::string_view query, std::string_view error, bool refused);

}
