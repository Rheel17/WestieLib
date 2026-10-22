#pragma once

#include <string>

#include <windows.h>

#include "ui/TrackCounts.h"
#include "ui/framework/Window.h"

namespace westie::ui {

class SyncedValue final : public framework::Window {
public:
    explicit SyncedValue(HWND parent);

    void set_font(HFONT font);
    void set_counts(const TrackCounts& counts);

protected:
    LRESULT handle_message(UINT message, WPARAM wparam, LPARAM lparam) override;

private:
    static const framework::WindowClass& window_class();

    void paint() const;

    TrackCounts counts_;
    std::wstring text_;
    HFONT font_ = nullptr;
};

}
