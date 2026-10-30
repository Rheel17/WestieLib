#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/matching/MatchWords.h"

namespace westie::tests {

namespace {

void check_words(
        const platform::TextNormalizer& normalizer, std::string_view text, const std::vector<std::string>& expected,
        std::source_location where = std::source_location::current()
) {
    std::vector<std::string> words = core::match_words(normalizer, text);
    check(words == expected, std::format("words of \"{}\": [{}], expected [{}]", text, words_text(words), words_text(expected)), where);
}

}

void check_match_words(const platform::TextNormalizer& normalizer) {
    check_words(normalizer, "Shut Up & Dance", {"shut", "up", "and", "dance"});
    check_words(normalizer, "Dan + Shay", {"dan", "and", "shay"});
    check_words(normalizer, "Ke$ha", {"kesha"});
    check_words(normalizer, "A$AP Rocky", {"asap", "rocky"});
    check_words(normalizer, "Stayin' Alive", {"stayin", "alive"});
    check_words(normalizer, "Don’t You", {"dont", "you"});
    check_words(normalizer, "The Jivin´ Jewels", {"the", "jivin", "jewels"});
    check_words(normalizer, "JAŸ-Z", {"jayz"});
    check_words(normalizer, "Re-Recorded", {"rerecorded"});
    check_words(normalizer, "Love Me - Live", {"love", "me", "live"});
    check_words(normalizer, "Love Me – Live", {"love", "me", "live"});
    check_words(normalizer, "WHISTLE -Japanese Version-", {"whistle", "japanese", "version"});
    check_words(normalizer, "Got To Give It Up (Pt. 1)", {"got", "to", "give", "it", "up", "part", "1"});
    check_words(normalizer, "Vol. 2", {"volume", "2"});
    check_words(normalizer, "Japanese ver.", {"japanese", "version"});
    check_words(normalizer, "Song ft. X", {"song", "feat", "x"});
    check_words(normalizer, "Song featuring X", {"song", "feat", "x"});
    check_words(normalizer, "CAZZETTE versus AronChupa", {"cazzette", "vs", "aronchupa"});
    check_words(normalizer, "Jason Derulo × Duke", {"jason", "derulo", "x", "duke"});
    check_words(normalizer, "Café Naïve", {"cafe", "naive"});
    check_words(normalizer, "ＢＬＡＣＫＰＩＮＫ", {"blackpink"});
    check_words(normalizer, "Moi...Lolita", {"moi", "lolita"});
    check_words(normalizer, "I Ain't Jivin', I'm Jammin' - 12\" Version", {"i", "aint", "jivin", "im", "jammin", "12", "version"});
    // NFKD takes the voiced mark off で as well, on both sides alike
    check_words(normalizer, "いつも何度でも（「千と千尋の神隠し」より）", {"いつも何度ても", "千と千尋の神隠し", "より"});
    CHECK(core::letters_of(core::match_words(normalizer, "Glass House")) == core::letters_of(core::match_words(normalizer, "Glasshouse")));
    CHECK(core::letters_of(core::match_words(normalizer, "Funky Town")) == "funkytown");
}

}
