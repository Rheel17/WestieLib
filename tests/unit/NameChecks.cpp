#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/matching/Credits.h"
#include "core/matching/MatchWords.h"
#include "core/matching/NameMatch.h"
#include "core/matching/TitleParts.h"

namespace westie::tests {

namespace {

std::string_view name_match_text(core::NameMatch match) {
    switch (match) {
    case core::NameMatch::Same:   return "same";
    case core::NameMatch::Starts: return "starts";
    case core::NameMatch::Inside: return "inside";
    case core::NameMatch::Other:  break;
    }
    return "other";
}

void check_name(
        const platform::TextNormalizer& normalizer, std::string_view a, std::string_view b, core::NameMatch expected,
        std::source_location where = std::source_location::current()
) {
    core::NameMatch match = core::name_match(core::match_words(normalizer, a), core::match_words(normalizer, b));
    check(match == expected, std::format("\"{}\" and \"{}\": {}, expected {}", a, b, name_match_text(match), name_match_text(expected)), where);
}

std::vector<std::vector<std::string>> names_of(const platform::TextNormalizer& normalizer, const std::vector<std::string_view>& names) {
    std::vector<std::vector<std::string>> words;
    for (std::string_view name : names) {
        words.push_back(core::match_words(normalizer, name));
    }
    return words;
}

void check_credit(
        const platform::TextNormalizer& normalizer, std::string_view part, const std::vector<std::string_view>& known, bool expected,
        std::source_location where = std::source_location::current()
) {
    bool credit = core::is_credit(core::match_words(normalizer, part), names_of(normalizer, known));
    check(credit == expected, std::format("\"{}\" {} a credit", part, expected ? "should be" : "shouldn't be"), where);
}

}

void check_names(const platform::TextNormalizer& normalizer) {
    check_name(normalizer, "Glass House", "Glasshouse", core::NameMatch::Same);
    check_name(normalizer, "The Jackson 5", "Jackson 5", core::NameMatch::Same);
    check_name(normalizer, "Ke$ha", "Kesha", core::NameMatch::Same);
    check_name(normalizer, "Prince", "Prince & The Revolution", core::NameMatch::Starts);
    check_name(normalizer, "Tora", "ToraMusic", core::NameMatch::Starts);
    check_name(normalizer, "Lizzo", "Lizzo Music", core::NameMatch::Starts);
    check_name(normalizer, "Lady A", "Lady Antebellum", core::NameMatch::Starts);
    check_name(normalizer, "Ms. Lauryn Hill", "Lauryn Hill", core::NameMatch::Inside);
    check_name(normalizer, "Phil Collins", "The Hits Of Phil Collins", core::NameMatch::Inside);
    check_name(normalizer, "J Balvin", "Balvin", core::NameMatch::Inside);
    check_name(normalizer, "Tones And I", "Tones Five", core::NameMatch::Other);
    check_name(normalizer, "Cass Elliot", "Mama Cass", core::NameMatch::Other);
    // Too short to start or sit in another name by its letters
    check_name(normalizer, "Ana", "Anastacia", core::NameMatch::Other);
    check_name(normalizer, "Ana", "Hannah", core::NameMatch::Other);

    check_credit(normalizer, "feat. Count Basie And His Orchestra", {"Frank Sinatra", "Count Basie"}, true);
    check_credit(normalizer, "feat. Gary Lightbody", {"Gary Lightbody of Snow Patrol"}, true);
    check_credit(normalizer, "feat. Jimin of BTS", {"JVKE", "Jimin"}, true);
    check_credit(normalizer, "Feki & Glasshouse Cover", {"Feki", "Glass House"}, true);
    check_credit(normalizer, "x Louis The Child", {"Jacob Banks", "Louis The Child"}, true);
    check_credit(normalizer, "feat. Earth, Wind & Fire & Hazel Monét", {"Victoria Monét", "Earth, Wind & Fire", "Hazel Monét"}, true);
    check_credit(normalizer, "Shawn Mendes & Justin Bieber", {"Shawn Mendes", "Justin Bieber"}, true);
    check_credit(normalizer, "cu Mario Fresh", {"Alina Eremia", "Mario Fresh"}, true);
    check_credit(normalizer, "feat. DaBaby", {"Dua Lipa"}, false);
    check_credit(normalizer, "StayLoose Remix", {"Bryce Fox", "StayLoose"}, false);
    check_credit(normalizer, "In Other Words", {"Frank Sinatra"}, false);
    check_credit(normalizer, "with Dubs", {"Sam Cooke"}, false);
    check_credit(normalizer, "with", {"Sam Cooke"}, false);

    core::TitleParts summer_days = core::title_parts(normalizer, "Summer Days (feat. Macklemore & Patrick Stump of Fall Out Boy)");
    std::vector<std::string> credited;
    for (const std::vector<std::string>& name : core::credited_names(summer_days)) {
        credited.push_back(words_text(name));
    }
    CHECK(credited == std::vector<std::string>{"macklemore and patrick stump of fall out boy", "macklemore", "patrick stump of fall out boy"});
    CHECK(core::is_credited_lead(core::match_words(normalizer, "Einaudi"), names_of(normalizer, {"Ludovico Einaudi"})));
    CHECK(!core::is_credited_lead(core::match_words(normalizer, "Swan Lake, Op. 20, Act 2"), names_of(normalizer, {"Pyotr Ilyich Tchaikovsky"})));
}

}
