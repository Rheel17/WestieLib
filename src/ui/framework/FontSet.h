#pragma once

#include "ui/framework/Dpi.h"
#include "ui/framework/MessageFont.h"

namespace westie::ui::framework {

class FontSet {
public:
    explicit FontSet(Dpi dpi);

    const MessageFont& regular() const noexcept;
    const MessageFont& semibold() const noexcept;

private:
    MessageFont regular_;
    MessageFont semibold_;
};

}
