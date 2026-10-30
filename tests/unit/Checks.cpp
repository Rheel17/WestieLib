#include "Checks.h"

#include <cstddef>
#include <format>
#include <iostream>

namespace westie::tests {

namespace {

int run = 0;
int failed = 0;

std::string_view file_name(std::string_view path) {
    std::size_t slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}

}

void check(bool passed, std::string_view what, std::source_location where) {
    ++run;
    if (!passed) {
        ++failed;
        std::cout << std::format("FAIL {}:{}: {}\n", file_name(where.file_name()), where.line(), what);
    }
}

int checks_run() {
    return run;
}

int checks_failed() {
    return failed;
}

std::string words_text(std::span<const std::string> words) {
    std::string text;
    for (const std::string& word : words) {
        if (!text.empty()) {
            text += ' ';
        }
        text += word;
    }
    return text;
}

}
