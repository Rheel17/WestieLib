#include "ui/framework/FontSet.h"

namespace westie::ui::framework {

FontSet::FontSet(Dpi dpi) : regular_(dpi), semibold_(dpi, FontStyle::Semibold) {}

const MessageFont& FontSet::regular() const noexcept {
    return regular_;
}

const MessageFont& FontSet::semibold() const noexcept {
    return semibold_;
}

}
