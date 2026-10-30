#pragma once

#include <source_location>
#include <span>
#include <string>
#include <string_view>

namespace westie::tests {

void check(bool passed, std::string_view what, std::source_location where = std::source_location::current());
int checks_run();
int checks_failed();
std::string words_text(std::span<const std::string> words);

}

#define CHECK(...) ::westie::tests::check(static_cast<bool>(__VA_ARGS__), #__VA_ARGS__)
