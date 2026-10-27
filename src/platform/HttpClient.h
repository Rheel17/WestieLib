#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <stop_token>
#include <string>
#include <vector>

namespace westie::platform {

struct HttpHeader {
    std::string name{};
    std::string value{};
};

class HttpClient {
public:
    virtual ~HttpClient() = default;

    virtual std::string get(const std::string& url, const std::vector<HttpHeader>& headers, const std::stop_token& stop) = 0;
    virtual std::string post(const std::string& url, const std::vector<HttpHeader>& headers, const std::string& body, const std::stop_token& stop) = 0;
    virtual void download(
            const std::string& url, const std::filesystem::path& file, const std::function<void(std::uint64_t received)>& progress, const std::stop_token& stop
    ) = 0;
};

}
