#pragma once

#include "core/events/CoreEvent.h"

namespace westie::core {

class EventSink {
public:
    virtual ~EventSink() = default;

    virtual void emit(CoreEvent event) = 0;
};

}
