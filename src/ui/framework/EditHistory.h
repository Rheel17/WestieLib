#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace westie::ui::framework {

struct EditSelection {
    std::size_t start = 0;
    std::size_t end = 0;

    bool operator==(const EditSelection&) const = default;
};

struct EditState {
    std::wstring text{};
    EditSelection selection{};
};

enum class EditKind { Typing, Deleting, Other };

class EditHistory {
public:
    explicit EditHistory(std::wstring text);

    const std::wstring& text() const noexcept;
    void reset(std::wstring text);
    void record(EditSelection before, EditState after, EditKind kind);
    std::optional<EditState> undo();
    std::optional<EditState> redo();

private:
    struct Step {
        EditState before{};
        EditState after{};
        EditKind kind = EditKind::Other;
    };

    std::wstring text_{};
    std::vector<Step> steps_{};
    std::size_t done_ = 0;
};

}
