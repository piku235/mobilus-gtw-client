#pragma once

#include "SocketEvents.h"

#include <cstdint>
#include <chrono>

namespace jungi::mobilus_gtw_client::io {

class SocketEventHandler {
public:
    virtual ~SocketEventHandler() = default;

    virtual SocketEvents socketEvents() = 0;
    virtual void handleSocketEvents(SocketEvents revents) = 0;
};

}
