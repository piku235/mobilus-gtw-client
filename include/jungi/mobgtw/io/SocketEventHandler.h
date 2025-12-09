#pragma once

#include "SocketEvents.h"

namespace jungi::mobgtw::io {

class SocketEventHandler {
public:
    virtual ~SocketEventHandler() = default;
    virtual SocketEvents socketEvents() = 0;
    virtual void handleSocketEvents(SocketEvents revents) = 0;
};

}
