#include "core/matching/CandidateSearch.h"

#include <exception>
#include <format>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/matching/CandidateSearchResults.h"
#include "core/matching/MatchCandidates.h"
#include "core/matching/MatchQueries.h"
#include "core/matching/SearchLogText.h"
#include "platform/Cancelled.h"

namespace westie::core {

CandidateSearch::CandidateSearch(
        YouTubeMusicSearch& search, const platform::TextNormalizer& normalizer, const StateStore& state, EventSink& events, Log& log
) :
        search_(search), normalizer_(normalizer), state_(state), events_(events), log_(log) {}

int CandidateSearch::search(std::string source_key, std::optional<std::string> query) {
    int request = ++requests_;
    // Assigning stops the search before this one and waits for it to end
    worker_ = std::jthread([this, request, source_key = std::move(source_key), query = std::move(query)](std::stop_token stop) {
        work(stop, request, source_key, query);
    });
    return request;
}

void CandidateSearch::stop() noexcept {
    worker_.request_stop();
}

void CandidateSearch::work(const std::stop_token& stop, int request, const std::string& source_key, const std::optional<std::string>& query) const {
    CandidateSearchResults answer{.request = request};
    std::vector<SongResult> results;
    LogSubject subject;
    // An exception that leaves a thread ends the process, so it becomes the answer instead
    try {
        std::shared_ptr<const State> state = state_.snapshot();
        std::vector<std::string> queries;
        if (auto source = state->sources.find(source_key); source != state->sources.end()) {
            subject = track_subject(*state, source->second.track);
            queries = query ? std::vector{*query} : match_queries(source->second);
        } else if (query) {
            queries.push_back(*query);
        }
        for (const std::string& searched : queries) {
            log_.write(LogLevel::Debug, search_started_text(searched), subject);
            std::vector<SongResult> found = search_.songs(searched, stop);
            log_.write(LogLevel::Debug, search_finished_text(searched, found.size()), subject);
            results.insert(results.end(), std::make_move_iterator(found.begin()), std::make_move_iterator(found.end()));
        }
    } catch (const platform::Cancelled&) {
        return;
    } catch (const std::exception& error) {
        answer.error = error.what();
        log_.write(LogLevel::Warning, std::format("Couldn't search YouTube Music: {}", answer.error), subject);
    }
    try {
        // The state may have changed while the search ran
        answer.candidates = match_candidates(*state_.snapshot(), normalizer_, source_key, results);
    } catch (const std::exception& error) {
        answer.error = error.what();
    }
    // Only a newer search or the dialog's close stops one, and neither waits for its answer
    if (!stop.stop_requested()) {
        events_.emit(std::move(answer));
    }
}

}
