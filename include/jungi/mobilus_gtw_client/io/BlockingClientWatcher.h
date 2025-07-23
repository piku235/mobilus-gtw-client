#pragma once

#include "ClientWatcher.h"

#include <chrono>

static constexpr int kInvalidSocketFd = -1;

namespace jungi::mobilus_gtw_client::io {

class BlockingClientWatcher : public ClientWatcher {
public:
    void loop();

    void watchTimer(TimerEventHandler* handler, std::chrono::milliseconds delay) override;
    void unwatchTimer(TimerEventHandler* handler) override;

    void watchSocket(SocketEventHandler* handler, int socketFd) override;
    void unwatchSocket(SocketEventHandler* handler, int socketFd) override;
private:
    SocketEventHandler* mSocketEventHandler = nullptr;
    TimerEventHandler* mTimerEventHandler = nullptr;
    int mSocketFd = kInvalidSocketFd;
    std::chrono::milliseconds mTimerDelay = std::chrono::milliseconds::max();
};

}
