#include <format>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/matching/TitleParts.h"

namespace westie::tests {

namespace {

// As "lead: name <with tail> [part] [part]"
std::string described(const core::TitleParts& title) {
    std::string text;
    if (!title.lead.empty()) {
        text = words_text(title.lead) + ": ";
    }
    text += words_text(title.name);
    if (!title.with_tail.empty()) {
        text += " <" + words_text(title.with_tail) + ">";
    }
    for (const std::vector<std::string>& part : title.parts) {
        text += " [" + words_text(part) + "]";
    }
    return text;
}

void check_title(
        const platform::TextNormalizer& normalizer, std::string_view title, std::string_view expected,
        std::source_location where = std::source_location::current()
) {
    std::string parts = described(core::title_parts(normalizer, title));
    check(parts == expected, std::format("parts of \"{}\": {}, expected {}", title, parts, expected), where);
}

}

void check_title_parts(const platform::TextNormalizer& normalizer) {
    check_title(normalizer, "Best Part", "best part");
    check_title(normalizer, "Best Part (feat. H.E.R.)", "best part [feat h e r]");
    check_title(normalizer, "willow - dancing witch version (Elvira remix)", "willow [dancing witch version] [elvira remix]");
    check_title(normalizer, "willow (dancing witch version (Elvira remix))", "willow [dancing witch version] [elvira remix]");
    check_title(
            normalizer, "Things Are Changin' (Solo Acoustic) [Live] (Solo Acoustic; Live)",
            "things are changin [solo acoustic] [live] [solo acoustic] [live]"
    );
    check_title(
            normalizer, "Walkin' Blues - Acoustic; Live at MTV Unplugged; 2013 Remaster", "walkin blues [acoustic] [live at mtv unplugged] [2013 remaster]"
    );
    check_title(normalizer, "My Baby Just Cares for Me (2013 - Remaster)", "my baby just cares for me [2013 remaster]");
    check_title(normalizer, "Can't Take My Eyes Off of You - (I Love You Baby)", "cant take my eyes off of you [i love you baby]");
    check_title(normalizer, "(I Can't Get No) Satisfaction", "satisfaction [i cant get no]");
    check_title(normalizer, "(Intro)", "intro");
    // Credits of their own
    check_title(normalizer, "Finesse - Remix; feat. Cardi B", "finesse [remix] [feat cardi b]");
    check_title(normalizer, "I'll Come Running Back To You - Single Version With Dubs", "ill come running back to you [single version] [with dubs]");
    check_title(normalizer, "Say Nothing (feat. MAY-A) (Ivy Lab Dub Mix) (feat. MAY-A)", "say nothing [feat maya] [ivy lab dub mix] [feat maya]");
    check_title(normalizer, "Song feat. X - Remix", "song [feat x] [remix]");
    // The work the song comes from, with what it holds
    check_title(normalizer, "Mr. Brightside - From \"Tell Me Lies (Season 2)\"", "mr brightside");
    check_title(normalizer, "Mr. Brightside - From “Tell Me Lies (Season 2)”", "mr brightside");
    check_title(normalizer, "The Wolf - From \"Fifty Shades Freed (Original Motion Picture Soundtrack)\"", "the wolf");
    check_title(normalizer, "Playground - from the series Arcane League of Legends (Baby Tate Remix)", "playground [baby tate remix]");
    check_title(normalizer, "It Had To Be You (from the Columbia film, When Harry Met Sally...)", "it had to be you");
    check_title(
            normalizer, "Popular (with Playboi Carti & Madonna) - From The Idol Vol. 1 (Music from the HBO Original Series)",
            "popular [with playboi carti and madonna]"
    );
    check_title(normalizer, "The Thrill Is Gone (Live From The United Kingdom/1993)", "the thrill is gone [live from the united kingdom 1993]");
    // A with tail and a lead, kept aside
    check_title(normalizer, "Retrograde with Glass House", "retrograde <with glass house>");
    check_title(normalizer, "Dance With Me", "dance <with me>");
    check_title(normalizer, "With or Without You", "with or without you");
    check_title(normalizer, "Einaudi: Experience", "einaudi: experience");
    check_title(normalizer, "Tchaikovsky: Swan Lake, Op. 20, Act 2: No. 10, Scene. Moderato", "tchaikovsky: swan lake op 20 act 2 no 10 scene moderato");
    CHECK(words_text(core::whole_name(core::title_parts(normalizer, "Retrograde with Glass House"))) == "retrograde with glass house");
    CHECK(words_text(core::whole_name(core::title_parts(normalizer, "Night Visions (Deluxe)"))) == "night visions");
}

}
