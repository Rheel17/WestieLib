#pragma once

#include <exception>

namespace westie::ui::framework {

class PendingException {
public:
    PendingException() = delete;

    static void capture() noexcept;
    static void rethrow_if_any();

private:
    static thread_local std::exception_ptr pending_;
};

}
