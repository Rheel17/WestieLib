#pragma once

namespace westie::ui {

enum class StatusTab { All, Synced, Waiting, NeedsReview, Failed, Removed, Kept };

struct StatusCounts {
    int all = 0;
    int synced = 0;
    int waiting = 0;
    int needs_review = 0;
    int failed = 0;
    int removed = 0;
    int kept = 0;
};

}
