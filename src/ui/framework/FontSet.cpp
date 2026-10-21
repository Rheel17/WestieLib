#include "ui/framework/FontSet.h"

namespace westie::ui::framework {

FontSet::FontSet(Dpi dpi) : regular_(dpi), semibold_(dpi, FontStyle::Semibold), heading_(dpi, FontStyle::Heading) {}

const MessageFont& FontSet::regular() const noexcept {
    return regular_;
}

const MessageFont& FontSet::semibold() const noexcept {
    return semibold_;
}

const MessageFont& FontSet::heading() const noexcept {
    return heading_;
}

}
