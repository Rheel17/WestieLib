#include "core/sync/DownloadPool.h"

#include <algorithm>
#include <exception>
#include <format>
#include <memory>
#include <utility>

#include "core/json/Utf8Path.h"
#include "core/log/LogLevel.h"
#include "core/log/LogSubject.h"
#include "core/log/LogText.h"
#include "core/state/TrackYtmIds.h"
#include "core/sync/ApplyDownload.h"
#include "core/sync/DownloadEvents.h"
#include "core/sync/TrackStatusChanged.h"
#include "core/sync/TracksToDownload.h"
#include "core/tools/ToolMissing.h"
#include "platform/Cancelled.h"

namespace westie::core {

namespace {

// What a download runs: yt-dlp, and the two it runs in turn
constexpr std::array DownloadTools = {ToolPackage::YtDlp, ToolPackage::Ffmpeg, ToolPackage::Deno};

std::string counted(std::size_t count, std::string_view thing) {
    return std::format("{} {}{}", count, thing, count == 1 ? "" : "s");
}

std::string tools_text(const std::vector<ToolPackage>& tools) {
    std::string text;
    for (std::size_t index = 0; index < tools.size(); ++index) {
        if (index > 0) {
            text += index + 1 == tools.size() ? " and " : ", ";
        }
        text += tool_package_name(tools[index]);
    }
    return text;
}

std::optional<std::string> ytm_id_of(const State& state, const std::string& track_key) {
    auto track = state.tracks.find(track_key);
    if (track == state.tracks.end()) {
        return std::nullopt;
    }
    std::optional<ResolvedYtmId> resolved = resolved_ytm_id(state, track->second);
    return resolved ? std::optional(std::move(resolved->ytm_id)) : std::nullopt;
}

}

DownloadPool::DownloadPool(DownloadServices services, std::filesystem::path work_folder, std::stop_token stop, std::function<void()> on_idle) :
        services_(services), work_folder_(std::move(work_folder)), stop_(std::move(stop)), on_idle_(std::move(on_idle)) {
    Settings settings = services_.settings.current();
    delay_ = settings.delay_between_tracks;
    for (int index = 0; index < std::clamp(settings.concurrency, MinConcurrency, MaxConcurrency); ++index) {
        workers_.emplace_back([this] { work(); });
    }
}

DownloadPool::~DownloadPool() {
    {
        std::scoped_lock lock(mutex_);
        closing_ = true;
    }
    wake_.notify_all();
}

void DownloadPool::add(std::vector<QueuedTrack> tracks) {
    DownloadsQueued queued;
    std::scoped_lock lock(mutex_);
    for (QueuedTrack& track : tracks) {
        if (queued_.insert(track.track).second) {
            queue_.push_back(track.track);
            queued.tracks.push_back(std::move(track));
        }
    }
    if (queued.tracks.empty()) {
        return;
    }
    services_.log.write(LogLevel::Debug, std::format("{} to download", counted(queued.tracks.size(), "track")));
    // Under the lock, so no worker reports on a track before the UI knows it is queued
    services_.events.emit(std::move(queued));
    ++changes_;
    wake_.notify_all();
}

void DownloadPool::forget(const std::vector<std::string>& tracks) {
    std::scoped_lock lock(mutex_);
    for (const std::string& track : tracks) {
        queued_.erase(track);
    }
}

bool DownloadPool::idle() {
    std::scoped_lock lock(mutex_);
    return queue_.empty() && active_ == 0;
}

void DownloadPool::work() {
    while (std::optional<Taken> taken = take()) {
        download(*taken);
        end(*taken);
    }
}

std::optional<DownloadPool::Taken> DownloadPool::take() {
    std::unique_lock lock(mutex_);
    while (!closing_ && !stop_.stop_requested()) {
        std::uint64_t seen = changes_;
        if (!waiting_for_tools_ && !queue_.empty()) {
            std::shared_ptr<const State> state = services_.state.snapshot();
            // A track whose id another worker is downloading waits for it, and the ones after it go first
            auto next = std::ranges::find_if(queue_, [&](const std::string& track) { return !busy_ids_.contains(ytm_id_of(*state, track).value_or("")); });
            if (next != queue_.end()) {
                std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
                if (now < next_start_) {
                    wake_.wait_until(lock, stop_, next_start_, [this] { return closing_; });
                    continue;
                }
                Taken taken{.track = *next, .ytm_id = ytm_id_of(*state, *next).value_or("")};
                queue_.erase(next);
                next_start_ = now + delay_;
                if (!taken.ytm_id.empty()) {
                    busy_ids_.insert(taken.ytm_id);
                }
                ++active_;
                return taken;
            }
        }
        wake_.wait(lock, stop_, [&] { return closing_ || changes_ != seen; });
    }
    return std::nullopt;
}

void DownloadPool::download(const Taken& taken) {
    std::shared_ptr<const State> state = services_.state.snapshot();
    auto track = state->tracks.find(taken.track);
    // Merged, gone with its playlist, or changed since it was queued; the event still ends it for the UI's counts
    if (track == state->tracks.end() || !needs_download(*state, track->second) || ytm_id_of(*state, taken.track) != taken.ytm_id) {
        services_.events.emit(TrackStatusChanged{.track = taken.track, .status = track == state->tracks.end() ? TrackStatus::Pending : track->second.status});
        return;
    }

    InstallCounts seen = install_counts();
    if (std::vector<ToolPackage> missing = missing_tools(); !missing.empty()) {
        wait_for_tools(taken, missing, seen);
        return;
    }
    std::filesystem::path work = work_folder_of(taken.track);
    services_.events.emit(TrackDownloading{.track = taken.track});
    // An exception that leaves a thread ends the process, so it becomes the track's failure instead
    try {
        remove_work_folder(work, taken.track);
        place(taken, work);
    } catch (const platform::Cancelled&) {
    } catch (const ToolMissing& missing) {
        // Removed or being replaced since it was looked for
        wait_for_tools(taken, {missing.package()}, seen);
    } catch (const std::exception& error) {
        fail(taken.track, error.what());
    }
    remove_work_folder(work, taken.track);
}

void DownloadPool::place(const Taken& taken, const std::filesystem::path& work) {
    std::shared_ptr<const State> state = services_.state.snapshot();
    Settings settings = services_.settings.current();
    PlacedTrack placed = services_.download.run(
            *state, taken.track, taken.ytm_id, settings, work,
            [&](int percent) { services_.events.emit(TrackProgress{.track = taken.track, .percent = percent}); }, stop_
    );

    bool kept = false;
    try {
        services_.state.update([&](State& changed) { kept = apply_download(changed, taken.track, taken.ytm_id, placed.file, placed.info); });
    } catch (...) {
        // A state that can't be saved doesn't know of the file, so the next try would add another
        services_.files.remove_file(settings.library_path / placed.file);
        throw;
    }
    LogSubject subject = track_subject(*services_.state.snapshot(), taken.track);
    if (!kept) {
        services_.files.remove_file(settings.library_path / placed.file);
        services_.events.emit(TrackStatusChanged{.track = taken.track});
        return;
    }
    services_.log.write(LogLevel::Info, std::format("Downloaded as \"{}\"", to_utf8(placed.file)), std::move(subject));
    services_.events.emit(TrackStatusChanged{.track = taken.track, .status = TrackStatus::Downloaded});
}

void DownloadPool::fail(const std::string& track, const std::string& error) {
    LogSubject subject = track_subject(*services_.state.snapshot(), track);
    services_.log.write(LogLevel::Error, std::format("Couldn't download: {}", one_line(error)), subject);
    try {
        services_.state.update([&](State& state) { apply_download_failure(state, track, error); });
    } catch (const std::exception& save_error) {
        // A state that can't be saved keeps the track as it was; the event still tells the UI
        services_.log.write(LogLevel::Error, std::format("Couldn't save the state: {}", save_error.what()), std::move(subject));
    }
    services_.events.emit(TrackStatusChanged{.track = track, .status = TrackStatus::Failed, .failure = TrackFailure::Download, .error = error});
}

// The track goes back to the front of the queue, and the pool starts no track until the tools are in
void DownloadPool::wait_for_tools(const Taken& taken, const std::vector<ToolPackage>& missing, const InstallCounts& seen) {
    bool first = false;
    {
        std::scoped_lock lock(mutex_);
        queue_.push_front(taken.track);
        first = !std::exchange(waiting_for_tools_, true);
    }
    if (!first) {
        return;
    }
    services_.log.write(LogLevel::Warning, std::format("Waiting for {} to install", tools_text(missing)));
    services_.events.emit(DownloadsWaiting{.waiting_for = missing});
    for (ToolPackage package : missing) {
        if (!services_.tools.wait_for_install_after(package, seen[std::to_underlying(package)], stop_)) {
            return;
        }
    }
    {
        std::scoped_lock lock(mutex_);
        waiting_for_tools_ = false;
    }
    services_.events.emit(DownloadsWaiting{});
    changed();
}

std::vector<ToolPackage> DownloadPool::missing_tools() const {
    std::vector<ToolPackage> missing;
    for (ToolPackage package : DownloadTools) {
        if (!services_.files.is_file(services_.tools.program(package))) {
            missing.push_back(package);
        }
    }
    return missing;
}

// Taken before the tools are looked for, so an install that comes in between still ends the wait
DownloadPool::InstallCounts DownloadPool::install_counts() {
    InstallCounts counts{};
    for (ToolPackage package : AllToolPackages) {
        counts[std::to_underlying(package)] = services_.tools.installs(package);
    }
    return counts;
}

void DownloadPool::end(const Taken& taken) {
    bool idle = false;
    {
        std::scoped_lock lock(mutex_);
        busy_ids_.erase(taken.ytm_id);
        --active_;
        ++changes_;
        idle = queue_.empty() && active_ == 0;
    }
    wake_.notify_all();
    if (idle) {
        on_idle_();
    }
}

void DownloadPool::changed() {
    {
        std::scoped_lock lock(mutex_);
        ++changes_;
    }
    wake_.notify_all();
}

std::filesystem::path DownloadPool::work_folder_of(const std::string& track) const {
    std::string name = track;
    std::ranges::replace(name, ':', '_');
    return work_folder_ / path_from_utf8(name);
}

void DownloadPool::remove_work_folder(const std::filesystem::path& work, const std::string& track) {
    try {
        services_.files.remove_tree(work);
    } catch (const std::exception& error) {
        services_.log.write(LogLevel::Debug, std::format("Couldn't remove the work folder: {}", error.what()), track_subject(*services_.state.snapshot(), track));
    }
}

}
