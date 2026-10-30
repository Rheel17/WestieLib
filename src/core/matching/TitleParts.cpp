#include "core/matching/TitleParts.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <utility>

#include "core/matching/MatchWords.h"

namespace westie::core {

namespace {

constexpr std::string_view SuffixSeparator = " - ";
constexpr std::string_view LeadSeparator = ": ";
constexpr char PartSeparator = ';';
// Folded text writes every kind of quote this way
constexpr char QuoteMark = '"';
constexpr std::string_view FeatWord = "feat";
constexpr std::string_view WithWord = "with";
constexpr std::string_view FromWord = "from";
constexpr std::string_view MusicWord = "music";

struct Bracket {
    // Without the brackets inside it, which are apart
    std::string text{};
    std::vector<Bracket> inner{};
    // Where it opened in the text around it
    std::size_t at = 0;
    // It opened inside quotes, so it belongs to the quoted name, as "(Season 2)" in From "Tell Me Lies (Season 2)"
    bool quoted = false;
};

bool opens(char c) {
    return c == '(' || c == '[' || c == '{';
}

bool closes(char c) {
    return c == ')' || c == ']' || c == '}';
}

Bracket read_bracket(std::string_view text, std::size_t& at, bool inside) {
    Bracket bracket;
    bool quoted = false;
    while (at < text.size()) {
        char c = text[at++];
        if (opens(c)) {
            // A space keeps the words on either side apart
            bracket.text += ' ';
            Bracket inner = read_bracket(text, at, true);
            inner.at = bracket.text.size();
            inner.quoted = quoted;
            bracket.inner.push_back(std::move(inner));
        } else if (closes(c) && inside) {
            return bracket;
        } else {
            quoted = c == QuoteMark ? !quoted : quoted;
            // A closing bracket that closes nothing separates words too
            bracket.text += closes(c) ? ' ' : c;
        }
    }
    return bracket;
}

bool names_work(const std::vector<std::string>& words) {
    // The work the song comes from, as "From "Frozen"" and "Music from the HBO Original Series"
    return (!words.empty() && words.front() == FromWord) || (words.size() > 1 && words[0] == MusicWord && words[1] == FromWord);
}

void add_part(std::vector<std::string> words, std::vector<std::vector<std::string>>& parts) {
    if (words.empty() || names_work(words)) {
        return;
    }
    // A feat or with inside starts a credit of its own, as in "Remix feat. Cardi B"
    std::vector<std::string> part;
    for (std::string& word : words) {
        if ((word == FeatWord || word == WithWord) && !part.empty()) {
            parts.push_back(std::exchange(part, {}));
        }
        part.push_back(std::move(word));
    }
    parts.push_back(std::move(part));
}

void add_text(std::string_view text, std::vector<std::vector<std::string>>& parts) {
    for (auto piece : text | std::views::split(PartSeparator)) {
        add_part(words_of(std::string_view(piece.begin(), piece.end())), parts);
    }
}

void add_bracket(const Bracket& bracket, std::vector<std::vector<std::string>>& parts) {
    // The work the song comes from goes with whatever it holds, as in "(From "Tarzan"/Soundtrack Version)"
    if (names_work(words_of(bracket.text))) {
        return;
    }
    add_text(bracket.text, parts);
    for (const Bracket& inner : bracket.inner) {
        add_bracket(inner, parts);
    }
}

void read_name(std::string_view text, TitleParts& title) {
    if (std::size_t lead = text.find(LeadSeparator); lead != std::string_view::npos) {
        title.lead = words_of(text.substr(0, lead));
        text.remove_prefix(lead + LeadSeparator.size());
    }
    std::vector<std::string> name = words_of(text);
    if (!name.empty()) {
        if (auto feat = std::find(std::next(name.begin()), name.end(), FeatWord); feat != name.end()) {
            title.parts.emplace_back(feat, name.end());
            name.erase(feat, name.end());
        }
    }
    // Past the first word only, so "With or Without You" keeps its name; whether the words after it credit someone
    // depends on the other song
    for (std::size_t at = name.size(); at-- > 1;) {
        if (name[at] == WithWord) {
            if (at + 1 < name.size()) {
                auto with = name.begin() + static_cast<std::ptrdiff_t>(at);
                title.with_tail.assign(with, name.end());
                name.erase(with, name.end());
            }
            break;
        }
    }
    title.name = std::move(name);
}

}

TitleParts title_parts(const platform::TextNormalizer& normalizer, std::string_view title) {
    std::string folded = folded_text(normalizer, title);
    std::size_t at = 0;
    Bracket root = read_bracket(folded, at, false);
    std::string_view text = root.text;
    std::vector<std::size_t> separators;
    for (std::size_t found = text.find(SuffixSeparator); found != std::string_view::npos; found = text.find(SuffixSeparator, found + SuffixSeparator.size())) {
        separators.push_back(found);
    }
    // Which piece of the text a bracket sits in: the name, or one of the suffixes
    auto piece_of = [&](const Bracket& bracket) { return std::ranges::count_if(separators, [&](std::size_t separator) { return separator < bracket.at; }); };

    TitleParts parts;
    read_name(text.substr(0, separators.empty() ? text.size() : separators.front()), parts);
    for (const Bracket& bracket : root.inner) {
        if (piece_of(bracket) == 0) {
            add_bracket(bracket, parts.parts);
        }
    }
    for (std::size_t index = 0; index < separators.size(); ++index) {
        std::size_t begin = separators[index] + SuffixSeparator.size();
        std::size_t end = index + 1 < separators.size() ? separators[index + 1] : text.size();
        std::string_view suffix = text.substr(begin, end - begin);
        // A suffix that names the work the song comes from goes with the brackets in its quotes, as in "- From "Fifty
        // Shades Freed (Original Motion Picture Soundtrack)"", but not with one after, as "(Baby Tate Remix)" in "- from
        // the series Arcane League of Legends (Baby Tate Remix)"
        bool work = names_work(words_of(suffix));
        if (!work) {
            add_text(suffix, parts.parts);
        }
        for (const Bracket& bracket : root.inner) {
            if (piece_of(bracket) == static_cast<std::ptrdiff_t>(index) + 1 && !(work && bracket.quoted)) {
                add_bracket(bracket, parts.parts);
            }
        }
    }
    // A title that is all brackets, as "(Intro)", is its own name
    if (parts.name.empty() && parts.lead.empty() && parts.with_tail.empty()) {
        parts.name = words_of(folded);
        parts.parts.clear();
    }
    return parts;
}

std::vector<std::string> whole_name(const TitleParts& title) {
    std::vector<std::string> name = title.lead;
    name.insert(name.end(), title.name.begin(), title.name.end());
    name.insert(name.end(), title.with_tail.begin(), title.with_tail.end());
    return name;
}

}
