#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client::io {

class TimerEventHandler {
public:
    virtual ~TimerEventHandler() = default;
    virtual void handleTimerEvent() = 0;
};

}
