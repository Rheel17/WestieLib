#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Checks.h"
#include "UnitChecks.h"
#include "core/matching/MatchEvidence.h"
#include "core/matching/MatchJudgement.h"
#include "core/matching/MatchRank.h"
#include "core/matching/SongFacts.h"
#include "core/state/MatchDoubt.h"

namespace westie::tests {

namespace {

constexpr core::MatchDoubt Title = core::MatchDoubt::Title;
constexpr core::MatchDoubt Artist = core::MatchDoubt::Artist;
constexpr core::MatchDoubt Version = core::MatchDoubt::Version;
constexpr core::MatchDoubt Length = core::MatchDoubt::Length;
constexpr core::MatchDoubt Explicit = core::MatchDoubt::Explicit;
constexpr core::MatchMismatch OtherSong = core::MatchMismatch::Song;
constexpr core::MatchMismatch OtherArtist = core::MatchMismatch::Artist;
constexpr core::MatchMismatch OtherRecording = core::MatchMismatch::Recording;
constexpr double Tolerance = 1e-9;

struct Song {
    std::string title{};
    std::vector<std::string> artists{};
    std::int64_t milliseconds = 0;
    std::string album{};
    std::optional<bool> explicit_content{};
};

struct Case {
    Song wanted{};
    Song found{};
    core::MatchVerdict verdict = core::MatchVerdict::Accept;
    core::MatchMismatch mismatch = core::MatchMismatch::None;
    std::vector<core::MatchDoubt> doubts{};
    std::source_location where{};
};

Song song(
        std::string title, std::vector<std::string> artists, std::int64_t milliseconds, std::string album = {},
        std::optional<bool> explicit_content = std::nullopt
) {
    return {
        .title = std::move(title),
        .artists = std::move(artists),
        .milliseconds = milliseconds,
        .album = std::move(album),
        .explicit_content = explicit_content,
    };
}

Case accepted(Song wanted, Song found, std::source_location where = std::source_location::current()) {
    return {.wanted = std::move(wanted), .found = std::move(found), .verdict = core::MatchVerdict::Accept, .where = where};
}

Case reviewed(Song wanted, Song found, std::vector<core::MatchDoubt> doubts, std::source_location where = std::source_location::current()) {
    return {.wanted = std::move(wanted), .found = std::move(found), .verdict = core::MatchVerdict::Review, .doubts = std::move(doubts), .where = where};
}

Case rejected(Song wanted, Song found, core::MatchMismatch mismatch, std::source_location where = std::source_location::current()) {
    return {.wanted = std::move(wanted), .found = std::move(found), .verdict = core::MatchVerdict::Reject, .mismatch = mismatch, .where = where};
}

core::SongFacts facts_of(const platform::TextNormalizer& normalizer, const Song& song) {
    return core::song_facts(normalizer, song.title, song.artists, song.album, std::chrono::milliseconds(song.milliseconds), song.explicit_content);
}

std::string song_text(const Song& song) {
    std::string artists;
    for (const std::string& artist : song.artists) {
        artists += artists.empty() ? artist : ", " + artist;
    }
    return std::format("\"{}\" by {}", song.title, artists);
}

std::string_view doubt_text(core::MatchDoubt doubt) {
    switch (doubt) {
    case core::MatchDoubt::Title:    return "title";
    case core::MatchDoubt::Artist:   return "artist";
    case core::MatchDoubt::Version:  return "version";
    case core::MatchDoubt::Length:   return "length";
    case core::MatchDoubt::Explicit: break;
    }
    return "explicit";
}

std::string_view mismatch_text(core::MatchMismatch mismatch) {
    switch (mismatch) {
    case core::MatchMismatch::Song:      return "song";
    case core::MatchMismatch::Artist:    return "artist";
    case core::MatchMismatch::Recording: return "recording";
    case core::MatchMismatch::None:      break;
    }
    return "none";
}

std::string judgement_text(core::MatchVerdict verdict, core::MatchMismatch mismatch, const std::vector<core::MatchDoubt>& doubts) {
    switch (verdict) {
    case core::MatchVerdict::Accept: return "accepted";
    case core::MatchVerdict::Reject: return std::format("rejected as another {}", mismatch_text(mismatch));
    case core::MatchVerdict::Review: break;
    }
    std::string text = "reviewed for";
    for (core::MatchDoubt doubt : doubts) {
        text += std::format(" {}", doubt_text(doubt));
    }
    return text;
}

void check_case(const platform::TextNormalizer& normalizer, const Case& expected) {
    core::MatchJudgement judgement = core::judge_match(facts_of(normalizer, expected.wanted), facts_of(normalizer, expected.found));
    std::string actual = judgement_text(judgement.verdict, judgement.mismatch, judgement.doubts);
    std::string wanted = judgement_text(expected.verdict, expected.mismatch, expected.doubts);
    check(actual == wanted, std::format("{} for {}: {}, expected {}", song_text(expected.found), song_text(expected.wanted), actual, wanted), expected.where);
}

void check_confidence(
        const platform::TextNormalizer& normalizer, const Song& wanted, const Song& found, double expected,
        std::source_location where = std::source_location::current()
) {
    double confidence = core::judge_match(facts_of(normalizer, wanted), facts_of(normalizer, found)).confidence;
    check(
            std::abs(confidence - expected) < Tolerance,
            std::format("confidence of {} for {}: {}, expected {}", song_text(found), song_text(wanted), confidence, expected), where
    );
}

void check_best(
        const platform::TextNormalizer& normalizer, const Song& wanted, const std::vector<Song>& candidates, std::size_t expected,
        const std::vector<bool>& known = {}, std::source_location where = std::source_location::current()
) {
    core::SongFacts facts = facts_of(normalizer, wanted);
    std::size_t best = 0;
    core::MatchRank best_rank;
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        core::MatchRank rank = core::match_rank(core::judge_match(facts, facts_of(normalizer, candidates[index])), index < known.size() && known[index]);
        // Ties keep the first, as they keep YouTube Music's order
        if (index == 0 || rank > best_rank) {
            best = index;
            best_rank = rank;
        }
    }
    check(best == expected, std::format("best for {}: #{}, expected #{}", song_text(wanted), best, expected), where);
}

std::string quoted_words(const std::vector<std::string>& words) {
    std::string text;
    for (const std::string& word : words) {
        text += std::format("{}\"{}\"", text.empty() ? "" : " ", word);
    }
    return std::format("[{}]", text);
}

void check_version_difference(
        const platform::TextNormalizer& normalizer, const Song& wanted, const Song& found, const std::vector<std::string>& wanted_words,
        const std::vector<std::string>& found_words, std::source_location where = std::source_location::current()
) {
    core::VersionDifference difference = core::version_difference(normalizer, wanted.title, wanted.artists, found.title, found.artists);
    check(
            difference.wanted == wanted_words && difference.found == found_words,
            std::format(
                    "version difference of {} and {}: {} {}, expected {} {}", song_text(wanted), song_text(found), quoted_words(difference.wanted),
                    quoted_words(difference.found), quoted_words(wanted_words), quoted_words(found_words)
            ),
            where
    );
}

}

void check_judgements(const platform::TextNormalizer& normalizer) {
    std::vector<Case> cases = {
        // The same recording, written differently
        accepted(
                song("Best Part", {"Daniel Caesar", "H.E.R."}, 209433, "Freudian"),
                song("Best Part (feat. H.E.R.)", {"Daniel Caesar", "H.E.R."}, 210000, "Freudian")
        ),
        accepted(song("Funkytown", {"Lipps Inc."}, 240000), song("Funky Town", {"Lipps Inc."}, 240000)),
        accepted(song("Until The Morning Light", {"David Benjamin"}, 188000), song("Until The Morninglight", {"David Benjamin"}, 189000)),
        accepted(song("Tik Tok", {"Ke$ha"}, 200000), song("TiK ToK", {"Kesha"}, 200000)),
        accepted(song("Shut Up And Dance - Acoustic", {"Tyler Ward"}, 144000), song("Shut Up & Dance (Acoustic Version)", {"Tyler Ward"}, 144000)),
        accepted(song("Got To Give It Up (Part 1)", {"Marvin Gaye"}, 250000), song("Got To Give It Up (Pt. 1)", {"Marvin Gaye"}, 250000)),
        accepted(
                song("Things Are Changin' - Solo Acoustic; Live", {"Gary Clark Jr."}, 250000),
                song("Things Are Changin' (Solo Acoustic) [Live] (Solo Acoustic; Live)", {"Gary Clark Jr."}, 250000)
        ),
        accepted(
                song("willow - dancing witch version (Elvira remix)", {"Taylor Swift", "Elvira"}, 200000),
                song("willow (dancing witch version (Elvira remix))", {"Taylor Swift"}, 200000)
        ),
        accepted(song("Dance With Me", {"Orleans"}, 200000), song("Dance With Me", {"Orleans"}, 200000)),
        accepted(song("Way down We Go", {"KALEO"}, 214000), song("Way Down We Go (Official Audio)", {"KALEO"}, 214000)),
        accepted(song("夜に駆ける", {"YOASOBI"}, 261000), song("夜に駆ける", {"YOASOBI"}, 261000)),
        accepted(song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, true), song("Kiss the Sky", {"Jason Derulo"}, 223000, {}, true)),
        // Edition and cut notes, which the length checks
        accepted(
                song("Fly Me To The Moon - 2008 Remastered", {"Frank Sinatra", "Count Basie"}, 147000),
                song("Fly Me To The Moon (2008 Remastered) (feat. Count Basie And His Orchestra)", {"Frank Sinatra"}, 148000)
        ),
        accepted(
                song("My Baby Just Cares for Me - 2013 Remastered Version", {"Nina Simone"}, 216000),
                song("My Baby Just Cares for Me (2013 - Remaster)", {"Nina Simone"}, 216000)
        ),
        accepted(song("The Beast - Digitally Remastered 95", {"Milt Buckner"}, 200000), song("The Beast", {"Milt Buckner"}, 202000)),
        accepted(song("I Ain't Jivin', I'm Jammin' - 12\" Version", {"Leon Huff"}, 355000), song("I Ain't Jivin', I'm Jammin'", {"Leon Huff"}, 356000)),
        accepted(
                song("There Is a Light That Never Goes Out - 2011 Remaster", {"The Smiths"}, 244000),
                song("There Is a Light That Never Goes Out (2017 Master)", {"The Smiths"}, 246000)
        ),
        accepted(song("Build Me Up Buttercup - Mono", {"The Foundations"}, 180100), song("Build Me Up Buttercup (Stereo)", {"The Foundations"}, 176000)),
        accepted(song("Dancing Machine", {"The Jackson 5"}, 161800, "Dancing Machine"), song("Dancing Machine (Single Version)", {"Jackson 5"}, 157000)),
        accepted(song("Strangers Like Me", {"Phil Collins"}, 181000), song("Strangers Like Me (From \"Tarzan\"/Soundtrack Version)", {"Phil Collins"}, 181000)),
        accepted(song("Sexy Love - Acoustic", {"Ne-Yo"}, 213600), song("Sexy Love (Acoustic)", {"Ne-Yo"}, 211000)),
        accepted(song("Movin' and a'Groovin'", {"Sam Cooke"}, 158300), song("Movin' and a'Groovin'", {"Sam Cooke"}, 155000)),
        accepted(song("Christmas Blues", {"Holly Cole"}, 177300), song("Christmas Blues (2022 Remastered)", {"Holly Cole"}, 174000)),
        accepted(song("Demons", {"Imagine Dragons"}, 175200, "Night Visions (Deluxe)"), song("Demons", {"Imagine Dragons"}, 178000)),
        // 5 to 10 s off, on the same album
        accepted(song("Let Me Love You", {"Mario"}, 249100, "Turning Point"), song("Let Me Love You", {"Mario"}, 257000, "Turning Point")),
        // Credits, however each side writes them
        accepted(song("Crazy In Love (feat. JAŸ-Z)", {"Beyoncé", "JAY-Z"}, 236000), song("Crazy In Love", {"Beyoncé"}, 236000)),
        accepted(
                song("Peaches (feat. Daniel Caesar & Giveon)", {"Justin Bieber", "Daniel Caesar", "Giveon"}, 198000), song("Peaches", {"Justin Bieber"}, 198000)
        ),
        accepted(
                song("Monster (Shawn Mendes & Justin Bieber)", {"Shawn Mendes", "Justin Bieber"}, 178000),
                song("Monster", {"Shawn Mendes", "Justin Bieber"}, 179000)
        ),
        accepted(song("Diddy Bop (x Louis The Child)", {"Jacob Banks", "Louis The Child"}, 200000), song("Diddy Bop", {"Jacob Banks"}, 200000)),
        accepted(
                song(
                        "Jiggle Jiggle (Jason Derulo x Duke & Jones x Louis Theroux x Amelia Dimz)",
                        {"Jason Derulo", "Duke & Jones", "Louis Theroux", "Amelia Dimz"}, 120000
                ),
                song("Jiggle Jiggle", {"Jason Derulo", "Duke & Jones", "Louis Theroux", "Amelia Dimz"}, 120000)
        ),
        accepted(
                song("Hollywood (feat. Earth, Wind & Fire + Hazel Monét)", {"Victoria Monét", "Earth, Wind & Fire", "Hazel Monét"}, 300000),
                song("Hollywood (feat. Earth, Wind & Fire & Hazel Monét)", {"Victoria Monét"}, 300000)
        ),
        accepted(song("Finesse - Remix; feat. Cardi B", {"Bruno Mars", "Cardi B"}, 217000), song("Finesse (Remix)", {"Bruno Mars", "Cardi B"}, 218000)),
        accepted(
                song("Summer Days (feat. Macklemore & Patrick Stump of Fall Out Boy)", {"Martin Garrix", "Macklemore", "Patrick Stump"}, 163000),
                song("Summer Days (feat. Patrick Stump of Fall Out Boy)", {"Martin Garrix", "Macklemore", "Fall Out Boy"}, 163000)
        ),
        accepted(
                song("The Last Time (feat. Gary Lightbody of Snow Patrol) (Taylor’s Version)", {"Taylor Swift", "Gary Lightbody"}, 299000),
                song("The Last Time (Taylor's Version) (feat. Gary Lightbody)", {"Taylor Swift"}, 300000)
        ),
        accepted(
                song("Left and Right (Feat. Jung Kook of BTS)", {"Charlie Puth", "Jung Kook"}, 154000),
                song("Left and Right (feat. Jung Kook)", {"Charlie Puth", "Jung Kook", "BTS"}, 154000)
        ),
        accepted(
                song("Angel Pt. 2", {"JVKE", "Jimin", "Charlie Puth", "Muni Long"}, 180000),
                song("Angel Pt. 2 (feat. Jimin of BTS)", {"JVKE", "Jimin", "Charlie Puth", "Muni Long"}, 181000)
        ),
        accepted(
                song(
                        "Popular (with Playboi Carti & Madonna) - From The Idol Vol. 1 (Music from the HBO Original Series)",
                        {"The Weeknd", "Madonna", "Playboi Carti"}, 215000
                ),
                song("Popular (From The Idol Vol. 1 (Music from the HBO Original Series)) (feat. Playboi Carti)", {"The Weeknd", "Madonna"}, 216000)
        ),
        accepted(
                song(
                        "Big Subwoofer (feat. Snoop Dogg, Ice Cube, E-40 & Too $hort ) - Single Version",
                        {"MOUNT WESTMORE", "Snoop Dogg", "Ice Cube", "E-40", "Too $hort"}, 200000
                ),
                song("Big Subwoofer (Single Version)", {"MOUNT WESTMORE", "Snoop Dogg", "Ice Cube", "E-40"}, 200000)
        ),
        accepted(song("Ai fost (cu Mario Fresh)", {"Alina Eremia", "Mario Fresh"}, 190000), song("Ai fost", {"Alina Eremia", "Mario"}, 190000)),
        accepted(
                song("Playground - from the series Arcane League of Legends (Baby Tate Remix)", {"Arcane", "Baby Tate", "Bea Miller"}, 160000),
                song("Playground (from the series Arcane League of Legends) (Baby Tate Remix)", {"Arcane"}, 160000)
        ),
        // Artist entries that hold several names, from either side
        accepted(song("Stay", {"Kurt Hugo Schneider", "Kina Grannis"}, 210000), song("Stay", {"Kina Grannis, Kurt Hugo Schneider"}, 211000)),
        accepted(song("I See Fire", {"Chance", "Kygo, a"}, 260000), song("I See Fire (feat. Kygo & a)", {"Chance"}, 261000)),
        accepted(
                song("Fifth Of Whiskey, Case Of The Blues", {"Mike Griffin & The Unknown Blues Band"}, 279000),
                song("Fifth of Whiskey, Case of the Blues (feat. The Unknown Blues Band)", {"Mike Griffin"}, 280000)
        ),
        accepted(
                song("Retrograde (Feki & Glasshouse Cover)", {"Feki", "Glass House"}, 220000),
                song("Retrograde with Glass House", {"Feki & Glass House"}, 220000)
        ),
        accepted(
                song("Experience", {"Ludovico Einaudi", "Daniel Hope", "I Virtuosi Italiani"}, 315000),
                song("Einaudi: Experience", {"Ludovico Einaudi", "Daniel Hope", "I Virtuosi Italiani"}, 316000)
        ),
        accepted(
                song(
                        "Tchaikovsky: Swan Lake, Op. 20, Act 2: No. 10, Scene. Moderato",
                        {"Pyotr Ilyich Tchaikovsky", "André Previn", "London Symphony Orchestra"}, 167000
                ),
                song("Swan Lake, Op. 20, Act 2: No. 10, Scene. Moderato", {"André Previn", "London Symphony Orchestra", "Pyotr Ilyich Tchaikovsky"}, 167000)
        ),
        // Artists named a little differently
        accepted(song("Inundated", {"Tora"}, 182000, "A Force Majeure"), song("Inundated", {"ToraMusic"}, 182000, "A Force Majeure")),
        accepted(song("About Damn Time", {"Lizzo"}, 191000), song("About Damn Time", {"Lizzo Music"}, 191000)),
        accepted(song("Need You Now", {"Lady A"}, 277000), song("Need You Now", {"Lady Antebellum"}, 278000)),
        accepted(
                song("The Sky Is Crying - 1985 Version", {"Stevie Ray Vaughan"}, 300000),
                song("The Sky Is Crying (1985 Version)", {"Stevie Ray Vaughan & Double Trouble"}, 300000)
        ),
        accepted(
                song("The Pink Panther Theme - From \"The Pink Panther\"", {"Henry Mancini", "Plas Johnson"}, 158000),
                song("The Pink Panther Theme (From \"The Pink Panther\")", {"Henry Mancini & His Orchestra", "Plas Johnson"}, 158000)
        ),
        accepted(song("Breathe", {"Olly Alexander (Years & Years)"}, 200000), song("Breathe", {"Olly Alexander"}, 201000)),
        accepted(song("Ain't Easy", {"Elijah Woods x Jamie Fine"}, 200000), song("Ain't Easy", {"Elijah Woods", "Jamie Fine"}, 200000)),
        accepted(
                song("Midnight Mischief (Tom Misch Remix)", {"Jordan Rakei", "Tom Misch"}, 300000),
                song("Midnight Mischief (Tom Misch Remix)", {"Jordan Rakei & Tom Misch"}, 301000)
        ),
        accepted(song("Strip That Down (feat. Quavo)", {"Liam Payne", "Quavo"}, 204000), song("Strip That Down", {"Liam Payne Official"}, 205000)),
        accepted(
                song("How Come U Don't Call Me Anymore", {"Scary Pockets", "Hunter", "Joe Bonamassa"}, 300000),
                song("How Come U Don't Call Me Anymore", {"Scary Pockets (ft. Hunter)"}, 301000)
        ),
        accepted(
                song("One Scotch, One Bourbon, One Beer", {"Little Willie Littlefield", "The Jivin' Jewels"}, 170000),
                song("One Scotch, One Bourbon, One Beer", {"Little Willie Littlefield", "The Jivin´ Jewels"}, 170000)
        ),
        // Only another of the Spotify artists, or a name inside another, which the album and the length bear out
        accepted(
                song("Perseverance - Ignacio Sanchez Remix", {"Ignacio Sanchez", "NuFects"}, 449000, "Perseverance (Ignacio Sanchez Remix)"),
                song("Perseverance (Ignacio Sanchez Remix)", {"Nufects"}, 449000, "Perseverance (Ignacio Sanchez Remix)")
        ),
        accepted(
                song("Doo Wop (That Thing)", {"Ms. Lauryn Hill"}, 320000, "The Miseducation of Lauryn Hill"),
                song("Doo Wop (That Thing)", {"Lauryn Hill"}, 320000, "The Miseducation of Lauryn Hill")
        ),

        // To review: another version of the same length, as a guest only YouTube Music names
        reviewed(song("Levitating", {"Dua Lipa"}, 203000), song("Levitating (feat. DaBaby)", {"Dua Lipa"}, 203000), {Version}),
        reviewed(song("Levitating", {"Dua Lipa"}, 203000), song("Levitating (feat. DaBaby)", {"Dua Lipa", "DaBaby"}, 203000), {Version}),
        reviewed(song("Lie To Me", {"5 Seconds of Summer"}, 220000), song("Lie To Me (feat. Julia Michaels)", {"5 Seconds of Summer"}, 220000), {Version}),
        reviewed(song("I Am Not My Hair", {"India.Arie"}, 280000), song("I Am Not My Hair (feat. Akon)", {"India.Arie"}, 282000), {Version}),
        reviewed(song("Genetics", {"Meghan Trainor"}, 200000), song("Genetics (feat. Pussycat Dolls)", {"Meghan Trainor"}, 200000), {Version}),
        reviewed(song("It Wasn't Me", {"Shaggy", "Rikrok"}, 227000), song("It Wasn't Me (feat. Ricardo Ducent)", {"Shaggy"}, 227000), {Version}),
        reviewed(song("Love U Like That - Korean Version", {"Lauv"}, 212000), song("Love U Like That", {"Lauv"}, 212000), {Version}),
        reviewed(
                song("Everything Has Changed (feat. Ed Sheeran) (Taylor’s Version)", {"Taylor Swift", "Ed Sheeran"}, 245000),
                song("Everything Has Changed (feat. Ed Sheeran)", {"Taylor Swift"}, 246000), {Version}
        ),
        reviewed(
                song(
                        "That’s Not How This Works (feat. Dan + Shay & Sabrina Carpenter) - Sabrina’s Version",
                        {"Charlie Puth", "Dan + Shay", "Sabrina Carpenter"}, 167000
                ),
                song("That’s Not How This Works (feat. Dan + Shay)", {"Charlie Puth"}, 166000), {Version}
        ),
        reviewed(song("Confetti (VF)", {"Charlotte Cardin"}, 180000), song("Confetti", {"Charlotte Cardin"}, 180000), {Version}),
        reviewed(song("Chocolate", {"SEREBRO"}, 200000), song("Chocolate (CIS version)", {"SEREBRO"}, 201000), {Version}),
        reviewed(
                song("This Town (feat. Sasha Alex Sloan & Gucci Mane) - Gucci Mane Edit", {"Kygo", "Sasha Alex Sloan", "Gucci Mane"}, 200000),
                song("This Town", {"Kygo"}, 201000), {Version}
        ),
        reviewed(song("Nena", {"YEИDRY"}, 200000), song("Nena - A COLORS SHOW", {"YEИDRY"}, 201000), {Version}),
        reviewed(
                song("Baecation - Ignant Mashup Remix", {"Loony T", "Tonio Armani"}, 153000),
                song("Baecation (DJ LoonyT Remix) (feat. Tonio Armani & DJ LoonyT)", {"Loony T"}, 154000), {Version}
        ),
        reviewed(
                song("Horns (StayLoose Remix)", {"Bryce Fox", "StayLoose"}, 204000), song("Horns (Arc North Remix)", {"Arc North", "Bryce Fox"}, 204000),
                {Version}
        ),
        reviewed(
                song("The Thrill Is Gone - Live", {"Gary Moore"}, 300000),
                song("The Thrill Is Gone (Live From The United Kingdom/1993)", {"Gary Moore"}, 300000), {Version}
        ),
        reviewed(
                song(
                        "Walkin' Blues - Acoustic; Live at MTV Unplugged, Bray Film Studios, Windsor, England, UK, 1/16/1992; 2013 Remaster", {"Eric Clapton"},
                        220000
                ),
                song("Walkin' Blues (Acoustic Live)", {"Eric Clapton"}, 220000), {Version}
        ),
        reviewed(
                song("Fly Me To The Moon (In Other Words)", {"Frank Sinatra", "Count Basie"}, 147000),
                song("Fly Me To The Moon (2008 Remastered) (feat. Count Basie And His Orchestra)", {"Frank Sinatra"}, 148000), {Version}
        ),
        reviewed(
                song("I'll Come Running Back To You - Single Version With Dubs", {"Sam Cooke", "The Soul Stirrers"}, 134000),
                song("I'll Come Running Back to You", {"Sam Cooke"}, 134000), {Version}
        ),
        reviewed(
                song("Blooz... First Thaingh 'Dis Moanin'", {"Wycliffe Gordon"}, 400000),
                song(
                        "Blooz... First Thaingh 'Dis Moanin' (feat. Victor Goines, Eric Reed) (feat. Victor Goines & Eric Reed)", {"Wycliffe Gordon"},
                        400000
                ),
                {Version}
        ),
        reviewed(
                song("いつも何度でも（「千と千尋の神隠し」より）", {"Youmi Kimura"}, 200000),
                song("いつも何度でも（千と千尋の神隠しより） - Itsumo Nandodemo", {"Youmi Kimura"}, 200000), {Version}
        ),
        // ...a title a word or a letter off
        reviewed(
                song("It Serves You Right to Suffer", {"John Lee Hooker"}, 308000), song("It Serve You Right To Suffer", {"John Lee Hooker"}, 308000), {Title}
        ),
        reviewed(
                song("Move Across the River", {"Bill Pinkney & The Original Drifters"}, 193000),
                song("Gonna Move Across The River", {"Bill Pinkney and the Original Drifters"}, 191000), {Title}
        ),
        // ...artists that may be others
        reviewed(
                song("Perseverance - Ignacio Sanchez Remix", {"Ignacio Sanchez", "NuFects"}, 449000),
                song("Perseverance (Ignacio Sanchez Remix)", {"Nufects"}, 449000), {Artist}
        ),
        reviewed(song("Doo Wop (That Thing)", {"Ms. Lauryn Hill"}, 320000), song("Doo Wop (That Thing)", {"Lauryn Hill"}, 320000), {Artist}),
        reviewed(song("Really", {"Mike Bloomfield", "Al Kooper"}, 300000), song("Really", {"Michael Bloomfield", "Al Kooper"}, 301000), {Artist}),
        reviewed(
                song("Sweater Weather", {"Kurt Hugo Schneider", "Alyson Stoner"}, 228000),
                song("Sweater Weather", {"Max Schneider Alyson Stoner  Max Schneider Alyson Stoner"}, 229000), {Artist}
        ),
        reviewed(song("In the Air Tonight", {"Phil Collins"}, 336000), song("In the Air Tonight", {"The Hits Of Phil Collins"}, 336000), {Artist}),
        reviewed(song("Sweet Lorraine - Remastered", {"Nat King Cole Trio"}, 187600), song("Sweet Lorraine", {"Nat King Cole"}, 191000), {Artist}),
        reviewed(
                song("Make Your Own Kind Of Music", {"Cass Elliot"}, 140000, "Make Your Own Kind Of Music"),
                song("Make Your Own Kind Of Music", {"Mama Cass"}, 140000, "Make Your Own Kind of Music"), {Artist}
        ),
        reviewed(
                song("Seven Nation Army", {"Postmodern Jukebox", "Haley Reinhart"}, 240000),
                song("Seven Nation Army (feat. Haley Reinahrt)", {"Scott Bradlee's Postmodern Jukebox"}, 241000), {Artist, Version}
        ),
        reviewed(
                song("Can't Take My Eyes Off of You - (I Love You Baby)", {"Ms. Lauryn Hill"}, 220000),
                song("Can't Take My Eyes Off Of You", {"Lauryn Hill"}, 221000), {Artist, Version}
        ),
        // ...another cut, or a length that can't say
        reviewed(song("Modern Loneliness - Single Version", {"Lauv"}, 225000), song("Modern Loneliness", {"LAUV"}, 253000), {Length}),
        reviewed(song("Stayin' Alive", {"Bee Gees"}, 93000), song("Stayin Alive", {"Bee Gees"}, 284000), {Length}),
        reviewed(
                song("Wicked Games - Radio Edit", {"Parra for Cuva", "Anna Naklab"}, 195000),
                song("Wicked Games (feat. Anna Naklab)", {"Parra for Cuva"}, 359000), {Length}
        ),
        reviewed(
                song("Somebody That I Used to Know", {"Gotye", "Kimbra"}, 246000),
                song("Somebody That I Used to Know", {"K3vin Envoy", "Gotye", "Kimbra"}, 363000), {Length}
        ),
        reviewed(
                song("Bloodstream", {"Ed Sheeran", "Rudimental"}, 308900, "Bloodstream"), song("Bloodstream", {"Ed Sheeran"}, 301000, "x (Deluxe Edition)"),
                {Length}
        ),
        reviewed(
                song("Green Onions", {"Booker T. & the M.G.'s"}, 167400, "Green Onions / Behave Yourself"),
                song("Green Onions", {"Booker T. & The M.G.'s"}, 176000, "Green Onions"), {Length}
        ),
        reviewed(
                song("Twerk It Like Miley", {"Brandon Beal", "Christopher"}, 208500, "Twerk It Like Miley"),
                song("Twerk It Like Miley (feat. Christopher)", {"Brandon Beal"}, 201000), {Length}
        ),
        reviewed(song("Song", {"Artist"}, 200000), song("Song", {"Artist"}, 0), {Length}),
        reviewed(song("Stole the Show", {"Kygo", "Parson James"}, 223200, "Cloud Nine"), song("Stole the Show", {"Parson James"}, 232000), {Artist, Length}),
        reviewed(
                song("It's A Man's, Man's, Man's World", {"James Brown"}, 175900),
                song("It's A Man's, Man's, Man's World", {"James Brown & The Famous Flames"}, 168000), {Artist, Length}
        ),
        reviewed(
                song("They Can't Take That Away From Me", {"Ella Fitzgerald & Louis Armstrong"}, 271100),
                song("They Can't Take That Away From Me", {"Ella Fitzgerald", "Louis Armstrong"}, 280000), {Artist, Length}
        ),
        reviewed(song("Mi Gente (feat. Beyoncé)", {"J Balvin", "Willy William", "Beyoncé"}, 210000), song("Mi Gente", {"Balvin"}, 183000), {Artist, Length}),
        // ...the clean version of an explicit song
        reviewed(song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, true), song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, false), {Explicit}),

        // Rejected: another song
        rejected(song("Trixie Blues", {"Trixie Smith"}, 193000), song("Praying Blues", {"Trixie Smith"}, 194000), OtherSong),
        rejected(song("Down Home Blues", {"Kenny"}, 180000), song("Bakersfield Blues", {"Kenny"}, 180000), OtherSong),
        rejected(song("WHISTLE", {"BLACKPINK"}, 212000), song("WHISTLE -Japanese Version-", {"BLACKPINK"}, 213000), OtherSong),
        rejected(song("Way down We Go", {"KALEO"}, 219600), song("KALEO - Way Down We Go (Official Video)", {"KALEO"}, 240000), OtherSong),
        rejected(song("Idol", {"YOASOBI"}, 213000), song("アイドル", {"YOASOBI"}, 213000), OtherSong),
        // ...the song by another artist
        rejected(song("Dance Monkey", {"Tones And I"}, 209400), song("Dance Monkey", {"Tones Five"}, 216000), OtherArtist),
        rejected(song("Tell Me", {"Rae"}, 200000), song("Tell Me (Karaoke)", {"Sing"}, 210000), OtherArtist),
        rejected(song("Make Your Own Kind Of Music", {"Cass Elliot"}, 140000), song("Make Your Own Kind Of Music", {"Mama Cass"}, 140000), OtherArtist),
        // ...another recording of the song, with a length of its own
        rejected(song("Crazy", {"Gnarls Barkley"}, 178000), song("Crazy (Live at Coachella 2006)", {"Gnarls Barkley"}, 211000), OtherRecording),
        rejected(song("Heaven", {"Bryan Adams"}, 243000), song("Heaven (Classic Version)", {"Bryan Adams"}, 264000), OtherRecording),
        rejected(
                song("Horns (StayLoose Remix)", {"Bryce Fox", "StayLoose"}, 204000), song("Horns (Arc North Remix)", {"Arc North", "Bryce Fox"}, 226000),
                OtherRecording
        ),
        rejected(song("Szerokie Wody", {"Mrozu"}, 267000), song("Szerokie wody (MTV Unplugged)", {"Mrozu"}, 575000), OtherRecording),
        rejected(
                song("Say Nothing (feat. MAY-A)", {"Flume", "MAY-A"}, 233000),
                song("Say Nothing (feat. MAY-A) (Ivy Lab Dub Mix) (feat. MAY-A)", {"Flume"}, 227000), OtherRecording
        ),
        rejected(song("willow - moonlit witch version", {"Taylor Swift"}, 208900), song("willow", {"Taylor Swift"}, 215000), OtherRecording),
        rejected(
                song("Where's My Love - French Version", {"SYML", "Lily Kershaw"}, 247800),
                song("Where's My Love (with Carla) (at Panoram Studios)", {"SYML"}, 240000), OtherRecording
        ),
        rejected(song("Ride It (Kya Yehi Pyaar Hai) - Hindi Version", {"Jay Sean"}, 193700), song("Ride It", {"Jay Sean"}, 191000), OtherRecording),
        rejected(song("Please Don't Go", {"Joel Adams"}, 210600), song("Please Don't Go (A Cappella)", {"Joel Adams"}, 207000), OtherRecording),
        rejected(
                song("Supermassive Black Hole - Twilight Soundtrack Version", {"Muse"}, 209500), song("Supermassive Black Hole", {"Muse"}, 213000),
                OtherRecording
        ),
        rejected(
                song("Can't Help Falling In Love - Live", {"UB40"}, 211000), song("Can't Help Falling in Love (With You) [Live]", {"UB40"}, 265000),
                OtherRecording
        ),
        rejected(song("Got To Give It Up (Part 1)", {"Marvin Gaye"}, 250000), song("Got To Give It Up (Pt. 2)", {"Marvin Gaye"}, 230000), OtherRecording),
    };
    for (const Case& expected : cases) {
        check_case(normalizer, expected);
    }

    // The confidence follows from the verdict, so the review threshold falls between Accept and Review
    check_confidence(normalizer, song("Best Part", {"Daniel Caesar"}, 209433), song("Best Part", {"Daniel Caesar"}, 210000), 1.0);
    check_confidence(normalizer, song("Inundated", {"Tora"}, 182000), song("Inundated", {"ToraMusic"}, 182000), 0.9);
    check_confidence(normalizer, song("Demons", {"Imagine Dragons"}, 175200), song("Demons", {"Imagine Dragons"}, 178000), 0.9);
    check_confidence(normalizer, song("Stayin' Alive", {"Bee Gees"}, 93000), song("Stayin Alive", {"Bee Gees"}, 284000), 0.75);
    check_confidence(normalizer, song("Stole the Show", {"Kygo", "Parson James"}, 223200), song("Stole the Show", {"Parson James"}, 232000), 0.65);
    check_confidence(normalizer, song("Song", {"A", "B"}, 200000, {}, true), song("Song (feat. C)", {"B"}, 200000, {}, false), 0.55);
    check_confidence(normalizer, song("Crazy", {"Gnarls Barkley"}, 178000), song("Crazy (Live at Coachella 2006)", {"Gnarls Barkley"}, 211000), 0.3);
    check_confidence(normalizer, song("Dance Monkey", {"Tones And I"}, 209400), song("Dance Monkey", {"Tones Five"}, 216000), 0.15);
    check_confidence(normalizer, song("Trixie Blues", {"Trixie Smith"}, 193000), song("Praying Blues", {"Trixie Smith"}, 194000), 0.05);

    // The best of several candidates
    check_best(
            normalizer, song("Levitating", {"Dua Lipa"}, 203000),
            {song("Levitating (feat. DaBaby)", {"Dua Lipa", "DaBaby"}, 203000), song("Levitating", {"Dua Lipa"}, 203000)}, 1
    );
    check_best(
            normalizer, song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, true),
            {song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, false), song("Kiss the Sky", {"Jason Derulo"}, 222000, {}, true)}, 1
    );
    check_best(
            normalizer, song("Crazy", {"Gnarls Barkley"}, 178000),
            {song("Crazy (Live at Coachella 2006)", {"Gnarls Barkley"}, 211000), song("Crazy", {"Gnarls Barkley"}, 178000)}, 1
    );
    check_best(
            normalizer, song("Way down We Go", {"KALEO"}, 219560, "Way down We Go"),
            {song("Way Down We Go", {"TommyMuzzic"}, 219000, "Way Down We Go"), song("Way down We Go", {"KALEO"}, 214000, "A/B")}, 1
    );
    check_best(normalizer, song("Song", {"Artist"}, 200000), {song("Song", {"Artist"}, 204000), song("Song", {"Artist"}, 201000)}, 1);
    check_best(normalizer, song("Song", {"Artist"}, 200000), {song("Song", {"Artist"}, 200000), song("Song", {"Artist"}, 200000)}, 1, {false, true});
    check_best(
            normalizer, song("Song", {"Artist"}, 200000, "Album"), {song("Song", {"Artist"}, 200000, "Single"), song("Song", {"Artist"}, 200000, "Album")}, 1
    );
    check_best(normalizer, song("Song", {"Artist"}, 200000), {song("Song", {"Artist"}, 200000), song("Song", {"Artist"}, 200000)}, 0);
    check_best(
            normalizer, song("Tell Me", {"Rae"}, 200000),
            {song("Hold Me", {"Rae"}, 200000), song("Tell Me", {"Sing"}, 200000), song("Tell Me (Live)", {"Rae"}, 260000)}, 2
    );

    // The words that tell two versions apart, as each title writes them
    check_version_difference(normalizer, song("Love U Like That", {"Lauv"}, 0), song("Love U Like That - Korean Version", {"Lauv"}, 0), {}, {"Korean"});
    check_version_difference(
            normalizer, song("Levitating", {"Dua Lipa"}, 0), song("Levitating (feat. DaBaby)", {"Dua Lipa", "DaBaby"}, 0), {}, {"feat.", "DaBaby"}
    );
    check_version_difference(
            normalizer, song("Shake It Off (Taylor's Version)", {"Taylor Swift"}, 0), song("Shake It Off", {"Taylor Swift"}, 0), {"Taylor's"}, {}
    );
    check_version_difference(
            normalizer, song("Hold On - StayLoose Remix", {"Bryce Fox", "StayLoose"}, 0), song("Hold On (Arc North Remix)", {"Bryce Fox"}, 0), {"StayLoose"},
            {"Arc", "North"}
    );
    check_version_difference(
            normalizer, song("Song", {"Artist"}, 0), song("Song (Solo Acoustic) [Live] (Solo Acoustic; Live) - Remastered 2011", {"Artist"}, 0), {},
            {"Solo", "Acoustic", "Live"}
    );
    check_version_difference(normalizer, song("Song (feat. Guest)", {"Artist", "Guest"}, 0), song("Song", {"Artist"}, 0), {}, {});
    check_version_difference(normalizer, song("Live Forever", {"Oasis"}, 0), song("Live Forever (Live)", {"Oasis"}, 0), {}, {"Live"});
    check_version_difference(normalizer, song("Song", {"Artist"}, 0), song("Song(Live) [Remix]", {"Artist"}, 0), {}, {"Live", "Remix"});
}

}
