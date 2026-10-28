#pragma once

#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "core/events/EventSink.h"
#include "core/state/StateStore.h"
#include "platform/HttpClient.h"
#include "platform/TextNormalizer.h"

namespace westie::core {

class VideoSearch {
public:
    VideoSearch(platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url, const StateStore& state, EventSink& events);

    // Without a query, it searches what the Matcher would. Returns the number the answer carries.
    int search(std::string source_key, std::optional<std::string> query);
    void stop() noexcept;

private:
    void work(const std::stop_token& stop, int request, const std::string& source_key, const std::optional<std::string>& query) const;

    platform::HttpClient& http_;
    const platform::TextNormalizer& normalizer_;
    std::string api_url_;
    const StateStore& state_;
    EventSink& events_;
    int requests_ = 0;
    // Last, so the thread is joined before the members it uses go
    std::jthread worker_;
};

}
