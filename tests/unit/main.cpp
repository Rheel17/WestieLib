#include <format>
#include <iostream>

#include "Checks.h"
#include "UnitChecks.h"
#include "platform/win32/Win32TextNormalizer.h"

int main() {
    westie::platform::win32::Win32TextNormalizer normalizer;
    westie::tests::check_match_words(normalizer);
    westie::tests::check_title_parts(normalizer);
    westie::tests::check_names(normalizer);
    westie::tests::check_judgements(normalizer);
    westie::tests::check_log_texts();
    westie::tests::check_run_phases();
    std::cout << std::format("{} checks, {} failure(s)\n", westie::tests::checks_run(), westie::tests::checks_failed());
    return westie::tests::checks_failed() == 0 ? 0 : 1;
}
