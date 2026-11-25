#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <set>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "core/events/EventSink.h"
#include "core/library/Library.h"
#include "core/log/Log.h"
#include "core/settings/SettingsStore.h"
#include "core/sync/TrackDownload.h"
#include "core/sync/TracksQueued.h"
#include "core/tools/ToolFolder.h"
#include "core/tools/ToolPackage.h"
#include "platform/FileOps.h"

namespace westie::core {

struct DownloadServices {
    const TrackDownload& download;
    ToolFolder& tools;
    platform::FileOps& files;
    const SettingsStore& settings;
    Library& library;
    EventSink& events;
    Log& log;
};

class DownloadPool {
public:
    DownloadPool(DownloadServices services, std::filesystem::path work_folder, std::stop_token stop, std::function<void()> on_idle);
    ~DownloadPool();

    DownloadPool(const DownloadPool&) = delete;
    DownloadPool& operator=(const DownloadPool&) = delete;

    void add(std::vector<QueuedTrack> tracks);
    // So a track tried already can be queued again
    void forget(const std::vector<std::string>& tracks);
    bool idle();

private:
    struct Taken {
        std::string track{};
        std::string ytm_id{};
    };

    using InstallCounts = std::array<std::uint64_t, AllToolPackages.size()>;

    void work();
    std::optional<Taken> take();
    void download(const Taken& taken);
    void place(const Taken& taken, const std::filesystem::path& work);
    void fail(const std::string& track, const std::string& error);
    void wait_for_tools(const Taken& taken, const std::vector<ToolPackage>& missing, const InstallCounts& seen);
    std::vector<ToolPackage> missing_tools() const;
    InstallCounts install_counts();
    void end(const Taken& taken);
    void changed();
    std::filesystem::path work_folder_of(const std::string& track) const;
    void remove_work_folder(const std::filesystem::path& work, const std::string& track);

    DownloadServices services_;
    std::filesystem::path work_folder_;
    std::stop_token stop_;
    std::function<void()> on_idle_;
    std::chrono::seconds delay_;
    std::mutex mutex_;
    std::condition_variable_any wake_;
    std::deque<std::string> queue_;
    // Every track the run queued, so none is queued twice
    std::set<std::string> queued_;
    // The YouTube Music ids being downloaded, so two tracks never download the same one at once
    std::set<std::string> busy_ids_;
    int active_ = 0;
    std::chrono::steady_clock::time_point next_start_{};
    bool waiting_for_tools_ = false;
    bool closing_ = false;
    // Counts every change a waiting worker may want to look at
    std::uint64_t changes_ = 0;
    // Last, so the threads are joined before the members they use go
    std::vector<std::jthread> workers_;
};

}
