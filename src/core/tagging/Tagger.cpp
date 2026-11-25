#include "core/tagging/Tagger.h"

#include <format>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <flacfile.h>
#include <flacpicture.h>
#include <opusfile.h>
#include <tbytevector.h>
#include <tstring.h>
#include <xiphcomment.h>

#include "core/json/Utf8Path.h"

namespace westie::core {

namespace {

constexpr std::string_view ArtistSeparator = ", ";
constexpr std::string_view JpegType = "image/jpeg";

TagLib::String text(std::string_view utf8) {
    return {std::string(utf8), TagLib::String::UTF8};
}

std::string joined(const std::vector<std::string>& parts) {
    std::string text;
    for (const std::string& part : parts) {
        if (!text.empty()) {
            text += ArtistSeparator;
        }
        text += part;
    }
    return text;
}

void add_text(TagLib::Ogg::XiphComment& comment, std::string_view key, std::string_view value) {
    if (!value.empty()) {
        comment.addField(text(key), text(value), false);
    }
}

// The comment holds only these afterwards, so nothing ffmpeg copied from the video stays
void write_comment(TagLib::Ogg::XiphComment& comment, const TrackTags& tags) {
    comment.removeAllFields();
    add_text(comment, "TITLE", tags.title);
    add_text(comment, "ARTIST", joined(tags.artists));
    add_text(comment, "ALBUM", tags.album);
    if (tags.year) {
        add_text(comment, "DATE", std::format("{}", *tags.year));
    }
    for (const std::string& source : tags.sources) {
        add_text(comment, "WESTIELIB_SOURCE", source);
    }
    add_text(comment, "WESTIELIB_YTMID", tags.ytm_id);
}

std::string field(const TagLib::Ogg::XiphComment& comment, std::string_view key) {
    const TagLib::Ogg::FieldListMap& fields = comment.fieldListMap();
    auto found = fields.find(text(key));
    return found == fields.end() || found->second.isEmpty() ? std::string() : found->second.front().to8Bit(true);
}

// The file may know what the state never did: a Spotify track's album and year come from the video's info
void update_comment(TagLib::Ogg::XiphComment& comment, TrackTags tags) {
    if (tags.album.empty()) {
        tags.album = field(comment, "ALBUM");
    }
    std::string date = tags.year ? std::string() : field(comment, "DATE");
    write_comment(comment, tags);
    add_text(comment, "DATE", date);
}

std::runtime_error unreadable(const std::filesystem::path& file) {
    return std::runtime_error(std::format("Couldn't read the tags of {}", to_utf8(file.filename())));
}

std::runtime_error unsaved(const std::filesystem::path& file) {
    return std::runtime_error(std::format("Couldn't write the tags of {}", to_utf8(file.filename())));
}

std::unique_ptr<TagLib::FLAC::Picture> front_cover(const std::string& jpeg) {
    auto picture = std::make_unique<TagLib::FLAC::Picture>();
    picture->setType(TagLib::FLAC::Picture::FrontCover);
    picture->setMimeType(text(JpegType));
    picture->setData(TagLib::ByteVector(jpeg.data(), static_cast<unsigned int>(jpeg.size())));
    return picture;
}

void write_opus(const std::filesystem::path& file, const TrackTags& tags) {
    TagLib::Ogg::Opus::File opus(file.c_str(), false);
    if (!opus.isValid() || opus.tag() == nullptr) {
        throw unreadable(file);
    }
    TagLib::Ogg::XiphComment& comment = *opus.tag();
    write_comment(comment, tags);
    comment.removeAllPictures();
    if (!tags.cover.empty()) {
        // The comment takes the picture over
        comment.addPicture(front_cover(tags.cover).release());
    }
    if (!opus.save()) {
        throw unsaved(file);
    }
}

void write_flac(const std::filesystem::path& file, const TrackTags& tags) {
    TagLib::FLAC::File flac(file.c_str(), false);
    TagLib::Ogg::XiphComment* comment = flac.isValid() ? flac.xiphComment(true) : nullptr;
    if (comment == nullptr) {
        throw unreadable(file);
    }
    write_comment(*comment, tags);
    flac.removePictures();
    if (!tags.cover.empty()) {
        // The file takes the picture over
        flac.addPicture(front_cover(tags.cover).release());
    }
    if (!flac.save()) {
        throw unsaved(file);
    }
}

// The front cover when there is one, else the first picture
std::string cover_of(const TagLib::List<TagLib::FLAC::Picture*>& pictures) {
    const TagLib::FLAC::Picture* chosen = nullptr;
    for (const TagLib::FLAC::Picture* picture : pictures) {
        if (chosen == nullptr || (picture->type() == TagLib::FLAC::Picture::FrontCover && chosen->type() != TagLib::FLAC::Picture::FrontCover)) {
            chosen = picture;
        }
    }
    if (chosen == nullptr) {
        return {};
    }
    TagLib::ByteVector data = chosen->data();
    return {data.data(), data.size()};
}

}

void write_tags(const std::filesystem::path& file, const TrackTags& tags) {
    if (file.extension() == L".flac") {
        write_flac(file, tags);
    } else {
        write_opus(file, tags);
    }
}

void update_tags(const std::filesystem::path& file, const TrackTags& tags) {
    if (file.extension() == L".flac") {
        TagLib::FLAC::File flac(file.c_str(), false);
        TagLib::Ogg::XiphComment* comment = flac.isValid() ? flac.xiphComment(true) : nullptr;
        if (comment == nullptr) {
            throw unreadable(file);
        }
        update_comment(*comment, tags);
        if (!flac.save()) {
            throw unsaved(file);
        }
        return;
    }
    TagLib::Ogg::Opus::File opus(file.c_str(), false);
    if (!opus.isValid() || opus.tag() == nullptr) {
        throw unreadable(file);
    }
    update_comment(*opus.tag(), tags);
    if (!opus.save()) {
        throw unsaved(file);
    }
}

std::string read_cover(const std::filesystem::path& file) {
    if (file.extension() == L".flac") {
        TagLib::FLAC::File flac(file.c_str(), false);
        return flac.isValid() ? cover_of(flac.pictureList()) : std::string();
    }
    TagLib::Ogg::Opus::File opus(file.c_str(), false);
    return opus.isValid() && opus.tag() != nullptr ? cover_of(opus.tag()->pictureList()) : std::string();
}

}
