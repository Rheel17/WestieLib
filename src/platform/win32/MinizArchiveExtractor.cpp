#include "platform/win32/MinizArchiveExtractor.h"

#include <format>
#include <stdexcept>
#include <string>

#include <windows.h>

#include <miniz.h>

#include "platform/Cancelled.h"
#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

using ZipReading = UniqueHandle<mz_zip_archive*, mz_zip_reader_end>;

struct Output {
    HANDLE file = nullptr;
    const std::stop_token* stop = nullptr;
};

std::size_t write_chunk(void* opaque, mz_uint64, const void* data, std::size_t size) {
    auto* output = static_cast<Output*>(opaque);
    DWORD written = 0;
    // miniz is C, so a failure is reported by writing less than it was given, never by throwing
    if (output->stop->stop_requested() || !WriteFile(output->file, data, static_cast<DWORD>(size), &written, nullptr)) {
        return 0;
    }
    return written;
}

std::string_view entry_file_name(std::string_view entry) {
    std::size_t slash = entry.find_last_of("/\\");
    return slash == std::string_view::npos ? entry : entry.substr(slash + 1);
}

std::string zip_error(mz_zip_archive& zip) {
    return mz_zip_get_error_string(mz_zip_get_last_error(&zip));
}

}

void MinizArchiveExtractor::extract(
        const std::filesystem::path& archive, std::span<const std::wstring_view> file_names, const std::filesystem::path& folder,
        const std::stop_token& stop
) const {
    std::string archive_name = to_utf8(archive.filename().wstring());
    // miniz opens files by narrow name, which can't hold every path, so it gets a FILE opened by the wide one
    UniqueCFile file(_wfopen(archive.c_str(), L"rb"));
    if (!file) {
        throw std::runtime_error(std::format("Couldn't open {}", archive_name));
    }
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_cfile(&zip, file.get(), 0, 0)) {
        throw std::runtime_error(std::format("Couldn't read {}: {}", archive_name, zip_error(zip)));
    }
    ZipReading reading(&zip);

    for (std::wstring_view file_name : file_names) {
        std::string wanted = to_utf8(file_name);
        mz_uint index = 0;
        mz_uint count = mz_zip_reader_get_num_files(&zip);
        mz_zip_archive_file_stat entry{};
        for (; index < count; ++index) {
            if (mz_zip_reader_file_stat(&zip, index, &entry) && !mz_zip_reader_is_file_a_directory(&zip, index)
                && entry_file_name(entry.m_filename) == wanted) {
                break;
            }
        }
        if (index == count) {
            throw std::runtime_error(std::format("{} has no {}", archive_name, wanted));
        }

        std::filesystem::path target = folder / file_name;
        UniqueFile output_file = adopt_file(CreateFileW(target.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (!output_file) {
            throw_last_error("CreateFileW");
        }
        Output output{.file = output_file.get(), .stop = &stop};
        if (!mz_zip_reader_extract_to_callback(&zip, index, write_chunk, &output, 0)) {
            if (stop.stop_requested()) {
                throw Cancelled();
            }
            throw std::runtime_error(std::format("Couldn't unpack {} from {}: {}", wanted, archive_name, zip_error(zip)));
        }
        check(FlushFileBuffers(output_file.get()), "FlushFileBuffers");
    }
}

}
