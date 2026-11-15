#pragma once

#include <span>
#include <vector>

#include <windows.h>

#include "ui/framework/Dpi.h"
#include "ui/framework/Rect.h"
#include "ui/framework/Size.h"

namespace westie::ui::framework {

enum class Anchor { Start, Stretch, End };

struct ControlAnchor {
    int id = 0;
    Anchor horizontal = Anchor::Start;
    Anchor vertical = Anchor::Start;
};

class DialogSizing {
public:
    // At WM_INITDIALOG, while the dialog has its template's size, which becomes its minimum
    DialogSizing(HWND dialog, std::span<const ControlAnchor> anchors);

    void resize(Size window_dips) const;
    Size window_dips() const;
    void limit(MINMAXINFO& info) const;
    void place() const;

private:
    struct Placed {
        HWND control = nullptr;
        Rect rect{};
        Anchor horizontal = Anchor::Start;
        Anchor vertical = Anchor::Start;
    };

    HWND dialog_ = nullptr;
    Dpi dpi_;
    Size client_{};
    Size minimum_dips_{};
    std::vector<Placed> controls_;
};

}
