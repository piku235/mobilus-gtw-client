#pragma once

#include "SocketEventHandler.h"
#include "TimerEventHandler.h"

#include <chrono>

namespace jungi::mobilus_gtw_client::io {

class ClientWatcher {
public:
    virtual ~ClientWatcher() = default;

    // TimerEventHandler
    virtual void watchTimer(TimerEventHandler* handler, std::chrono::milliseconds delay) = 0;
    virtual void unwatchTimer(TimerEventHandler* handler) = 0;

    // SocketEventHandler
    virtual void watchSocket(SocketEventHandler* handler, int socketFd) = 0;
    virtual void unwatchSocket(SocketEventHandler* handler, int socketFd) = 0;
};

}
