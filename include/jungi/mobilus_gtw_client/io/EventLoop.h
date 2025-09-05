#pragma once

#include "SocketEvents.h"
#include "SocketEventHandler.h"

#include <chrono>
#include <cstdint>

namespace jungi::mobilus_gtw_client::io {

class EventLoop {
public:
    using TimerCallback = void (*)(void*);

    virtual ~EventLoop() = default;
    virtual void startTimer(std::chrono::milliseconds delay, TimerCallback callback, void* callbackData) = 0;
    virtual void stopTimer(TimerCallback callback, void* callbackData) = 0;
    virtual void watchSocket(int socketFd, SocketEventHandler* handler) = 0;
    virtual void unwatchSocket(int socketFd) = 0;
};

}
