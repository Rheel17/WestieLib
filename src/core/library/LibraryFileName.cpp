#include "core/library/LibraryFileName.h"

#include <cctype>
#include <cstddef>
#include <format>
#include <vector>

#include "core/library/WindowsNames.h"

namespace westie::core {

namespace {

constexpr std::size_t MaxStemCharacters = 200;
constexpr std::string_view NotInNames = "<>:\"/\\|?*";
constexpr std::string_view ArtistSeparator = ", ";
constexpr std::string_view TitleSeparator = " - ";

bool is_continuation_byte(char c) {
    return (static_cast<unsigned char>(c) & 0xC0) == 0x80;
}

std::string replace_not_allowed(std::string_view text) {
    std::string name;
    for (char c : text) {
        bool control = static_cast<unsigned char>(c) < 0x20 || c == 0x7F;
        name += control || NotInNames.contains(c) ? '_' : c;
    }
    return name;
}

void trim_end(std::string& name) {
    while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
        name.pop_back();
    }
}

// Characters, not bytes, so a name never ends halfway through one
void cap(std::string& name) {
    std::size_t characters = 0;
    for (std::size_t index = 0; index < name.size(); ++index) {
        if (!is_continuation_byte(name[index]) && ++characters > MaxStemCharacters) {
            name.resize(index);
            return;
        }
    }
}

std::string joined(const std::vector<std::string>& parts, std::string_view separator) {
    std::string text;
    for (const std::string& part : parts) {
        if (!text.empty()) {
            text += separator;
        }
        text += part;
    }
    return text;
}

}

std::string library_file_stem(const TrackMetadata& metadata, std::string_view ytm_id) {
    std::string artists = joined(metadata.artists, ArtistSeparator);
    std::string stem = metadata.title.empty() ? std::string(ytm_id) : artists.empty() ? metadata.title : artists + std::string(TitleSeparator) + metadata.title;
    stem = replace_not_allowed(stem);
    trim_end(stem);
    cap(stem);
    trim_end(stem);
    if (stem.empty()) {
        stem = replace_not_allowed(ytm_id);
    }
    if (windows_name_problem(stem) == NameProblem::ReservedName) {
        stem += '_';
    }
    return stem;
}

std::string numbered_file_name(std::string_view stem, std::string_view extension, int number) {
    if (number <= 1) {
        return std::format("{}.{}", stem, extension);
    }
    return std::format("{} ({}).{}", stem, number, extension);
}

// Windows compares names without case; ASCII is enough here, since the move refuses a name that is taken anyway
std::string file_name_key(std::string_view name) {
    std::string key;
    for (char c : name) {
        key += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return key;
}

std::string free_file_name(std::string_view stem, std::string_view extension, const std::set<std::string>& taken_keys, int first_number) {
    for (int number = first_number;; ++number) {
        std::string name = numbered_file_name(stem, extension, number);
        if (!taken_keys.contains(file_name_key(name))) {
            return name;
        }
    }
}

}
