#pragma once

#include <mutex>
#include <vector>

#include <windows.h>

#include "core/events/CoreEvent.h"
#include "core/events/EventSink.h"

namespace westie::ui {

constexpr UINT CoreEventsMessage = WM_APP + 1;

class CoreEventQueue final : public core::EventSink {
public:
    void set_target(HWND target);
    void emit(core::CoreEvent event) override;
    std::vector<core::CoreEvent> take();

private:
    std::mutex mutex_;
    HWND target_ = nullptr;
    std::vector<core::CoreEvent> events_;
};

}
