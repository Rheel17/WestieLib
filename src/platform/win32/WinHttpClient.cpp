#include "platform/win32/WinHttpClient.h"

#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

#include <windows.h>
#include <winhttp.h>

#include "platform/Cancelled.h"
#include "platform/win32/Utf16.h"
#include "platform/win32/Win32Error.h"

namespace westie::platform::win32 {

namespace {

constexpr int ResolveTimeoutMs = 15'000;
constexpr int ConnectTimeoutMs = 15'000;
constexpr int SendTimeoutMs = 30'000;
constexpr int ReceiveTimeoutMs = 30'000;
constexpr std::size_t ChunkBytes = 64 * 1024;
constexpr DWORD FirstSuccessStatus = 200;
constexpr DWORD FirstFailureStatus = 300;

std::string error_text(DWORD error) {
    // WinHTTP's messages are in winhttp.dll, not the system's table
    PWSTR buffer = nullptr;
    DWORD length = FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            GetModuleHandleW(L"winhttp.dll"), error, 0, reinterpret_cast<PWSTR>(&buffer), 0, nullptr
    );
    UniqueLocalString owned(buffer);
    if (length == 0) {
        return std::format("error {}", error);
    }
    std::wstring_view text(buffer, length);
    while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ' || text.back() == L'.')) {
        text.remove_suffix(1);
    }
    return to_utf8(text);
}

[[noreturn]] void fail(const std::stop_token& stop, const std::wstring& host) {
    DWORD error = GetLastError();
    // A stop request closes the request handle, so a call that fails after one was cancelled
    if (stop.stop_requested()) {
        throw Cancelled();
    }
    throw std::runtime_error(std::format("Couldn't reach {}: {}", to_utf8(host), error_text(error)));
}

}

WinHttpClient::WinHttpClient(const std::wstring& user_agent) :
        session_(
                check(WinHttpOpen(user_agent.c_str(), WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0), "WinHttpOpen")
        ) {
    check(WinHttpSetTimeouts(session_.get(), ResolveTimeoutMs, ConnectTimeoutMs, SendTimeoutMs, ReceiveTimeoutMs), "WinHttpSetTimeouts");
}

std::string WinHttpClient::get(const std::string& url, const std::stop_token& stop) {
    std::string body;
    request(url, [&body](std::span<const char> data) { body.append(data.begin(), data.end()); }, stop);
    return body;
}

void WinHttpClient::download(
        const std::string& url, const std::filesystem::path& file, const std::function<void(std::uint64_t received)>& progress, const std::stop_token& stop
) {
    UniqueFile output = adopt_file(CreateFileW(file.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!output) {
        throw_last_error("CreateFileW");
    }
    std::uint64_t received = 0;
    request(
            url,
            [&](std::span<const char> data) {
                DWORD written = 0;
                check(WriteFile(output.get(), data.data(), static_cast<DWORD>(data.size()), &written, nullptr), "WriteFile");
                received += data.size();
                progress(received);
            },
            stop
    );
    check(FlushFileBuffers(output.get()), "FlushFileBuffers");
}

void WinHttpClient::request(const std::string& url, const std::function<void(std::span<const char>)>& on_data, const std::stop_token& stop) {
    std::wstring wide_url = to_utf16(url);
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);
    check(WinHttpCrackUrl(wide_url.c_str(), 0, 0, &parts), "WinHttpCrackUrl");
    std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    // The query follows the path in the URL, so one string holds both
    std::wstring path(parts.lpszUrlPath, parts.dwUrlPathLength + parts.dwExtraInfoLength);

    UniqueInternet connection(WinHttpConnect(session_.get(), host.c_str(), parts.nPort, 0));
    if (!connection) {
        fail(stop, host);
    }
    DWORD flags = parts.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0;
    UniqueInternet request(WinHttpOpenRequest(connection.get(), L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags));
    if (!request) {
        fail(stop, host);
    }

    // Closing the handle from the stopping thread ends a call that is blocked on it. The calls below use a copy, so they
    // fail rather than race with the reset.
    HINTERNET handle = request.get();
    std::stop_callback cancel(stop, [&request] { request.reset(); });
    if (!WinHttpSendRequest(handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) || !WinHttpReceiveResponse(handle, nullptr)) {
        fail(stop, host);
    }

    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(
                handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX
        )) {
        fail(stop, host);
    }
    if (status < FirstSuccessStatus || status >= FirstFailureStatus) {
        throw std::runtime_error(std::format("HTTP {} from {}", status, to_utf8(host)));
    }

    // A synchronous read waits until it fills the buffer, so read what has arrived, and progress keeps up with a slow
    // connection
    std::vector<char> chunk(ChunkBytes);
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(handle, &available)) {
            fail(stop, host);
        }
        if (stop.stop_requested()) {
            throw Cancelled();
        }
        if (available == 0) {
            return;
        }
        DWORD read = 0;
        if (!WinHttpReadData(handle, chunk.data(), std::min(available, static_cast<DWORD>(chunk.size())), &read)) {
            fail(stop, host);
        }
        on_data({chunk.data(), read});
    }
}

}
