#include "platform/win32/BCryptHasher.h"

#include <array>
#include <format>
#include <vector>

#include <windows.h>
#include <bcrypt.h>

#include "platform/win32/UniqueHandle.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr std::size_t ChunkBytes = 1 << 20;
constexpr std::size_t Sha256Bytes = 32;

}

std::string BCryptHasher::sha256(const std::filesystem::path& file) const {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    check_ntstatus(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0), "BCryptOpenAlgorithmProvider");
    UniqueAlgorithmProvider owned_algorithm(algorithm);
    BCRYPT_HASH_HANDLE hash = nullptr;
    check_ntstatus(BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0), "BCryptCreateHash");
    UniqueHash owned_hash(hash);

    UniqueFile input = adopt_file(CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
    if (!input) {
        throw_last_error("CreateFileW");
    }
    std::vector<UCHAR> chunk(ChunkBytes);
    while (true) {
        DWORD read = 0;
        check(ReadFile(input.get(), chunk.data(), static_cast<DWORD>(chunk.size()), &read, nullptr), "ReadFile");
        if (read == 0) {
            break;
        }
        check_ntstatus(BCryptHashData(hash, chunk.data(), read, 0), "BCryptHashData");
    }

    std::array<UCHAR, Sha256Bytes> digest{};
    check_ntstatus(BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0), "BCryptFinishHash");
    std::string text;
    for (UCHAR byte : digest) {
        text += std::format("{:02x}", byte);
    }
    return text;
}

}
