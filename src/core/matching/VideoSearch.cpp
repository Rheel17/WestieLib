#include "core/matching/VideoSearch.h"

#include <exception>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "core/matching/MatchQueries.h"
#include "core/matching/VideoCandidates.h"
#include "core/matching/VideoSearchResults.h"
#include "core/matching/YouTubeMusicSearch.h"
#include "platform/Cancelled.h"

namespace westie::core {

VideoSearch::VideoSearch(
        platform::HttpClient& http, const platform::TextNormalizer& normalizer, std::string api_url, const StateStore& state, EventSink& events
) :
        http_(http), normalizer_(normalizer), api_url_(std::move(api_url)), state_(state), events_(events) {}

int VideoSearch::search(std::string source_key, std::optional<std::string> query) {
    int request = ++requests_;
    // Assigning stops the search before this one and waits for it to end
    worker_ = std::jthread([this, request, source_key = std::move(source_key), query = std::move(query)](std::stop_token stop) {
        work(stop, request, source_key, query);
    });
    return request;
}

void VideoSearch::stop() noexcept {
    worker_.request_stop();
}

void VideoSearch::work(const std::stop_token& stop, int request, const std::string& source_key, const std::optional<std::string>& query) const {
    VideoSearchResults answer{.request = request};
    std::vector<SongResult> results;
    // An exception that leaves a thread ends the process, so it becomes the answer instead
    try {
        std::shared_ptr<const State> state = state_.snapshot();
        std::vector<std::string> queries;
        if (query) {
            queries.push_back(*query);
        } else if (auto source = state->sources.find(source_key); source != state->sources.end()) {
            queries = match_queries(source->second);
        }
        for (const std::string& searched : queries) {
            std::vector<SongResult> found = search_youtube_music_songs(http_, api_url_, searched, stop);
            results.insert(results.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
        }
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        answer.error = error.what();
    }
    try {
        // The state may have changed while the search ran
        answer.candidates = video_candidates(*state_.snapshot(), normalizer_, source_key, results);
    } catch (const std::exception& error) {
        answer.error = error.what();
    }
    // Only a newer search or the dialog's close stops one, and neither waits for its answer
    if (!stop.stop_requested()) {
        events_.emit(std::move(answer));
    }
}

}
