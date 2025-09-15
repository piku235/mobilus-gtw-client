#pragma once

#include "SocketEventHandler.h"
#include "SocketEvents.h"

#include <chrono>

namespace jungi::mobilus_gtw_client::io {

class EventLoop {
public:
    using TimerId = int;
    using TimerCallback = void (*)(void*);

    static constexpr TimerId kInvalidTimerId = -1;

    virtual ~EventLoop() = default;
    virtual TimerId startTimer(std::chrono::milliseconds delay, TimerCallback callback, void* callbackData) = 0;
    virtual void stopTimer(TimerId id) = 0;
    virtual void watchSocket(int socketFd, SocketEventHandler* handler) = 0;
    virtual void unwatchSocket(int socketFd) = 0;
};

}
