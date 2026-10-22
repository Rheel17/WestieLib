#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <span>
#include <stop_token>
#include <string>

#include "platform/HttpClient.h"
#include "platform/win32/UniqueHandle.h"

namespace westie::platform::win32 {

class WinHttpClient final : public HttpClient {
public:
    explicit WinHttpClient(const std::wstring& user_agent);

    std::string get(const std::string& url, const std::stop_token& stop) override;
    void download(
            const std::string& url, const std::filesystem::path& file, const std::function<void(std::uint64_t received)>& progress, const std::stop_token& stop
    ) override;

private:
    void request(const std::string& url, const std::function<void(std::span<const char>)>& on_data, const std::stop_token& stop);

    UniqueInternet session_;
};

}
