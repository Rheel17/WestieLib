#include "ui/framework/EditHistory.h"

#include <cstddef>
#include <utility>

namespace westie::ui::framework {

EditHistory::EditHistory(std::wstring text) : text_(std::move(text)) {}

const std::wstring& EditHistory::text() const noexcept {
    return text_;
}

void EditHistory::reset(std::wstring text) {
    text_ = std::move(text);
    steps_.clear();
    done_ = 0;
}

void EditHistory::record(EditSelection before, EditState after, EditKind kind) {
    // After an undo the next change starts its own step, even where it continues the one before
    bool continues = kind != EditKind::Other && done_ > 0 && done_ == steps_.size() && steps_.back().kind == kind && steps_.back().after.selection == before;
    steps_.erase(steps_.begin() + static_cast<std::ptrdiff_t>(done_), steps_.end());
    if (continues) {
        steps_.back().after = after;
    } else {
        steps_.push_back({.before = {.text = text_, .selection = before}, .after = after, .kind = kind});
    }
    text_ = std::move(after.text);
    done_ = steps_.size();
}

std::optional<EditState> EditHistory::undo() {
    if (done_ == 0) {
        return std::nullopt;
    }
    --done_;
    text_ = steps_[done_].before.text;
    return steps_[done_].before;
}

std::optional<EditState> EditHistory::redo() {
    if (done_ == steps_.size()) {
        return std::nullopt;
    }
    const Step& step = steps_[done_++];
    text_ = step.after.text;
    return step.after;
}

}
