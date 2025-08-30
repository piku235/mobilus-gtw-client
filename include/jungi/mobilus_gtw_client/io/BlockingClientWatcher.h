#pragma once

#include "ClientWatcher.h"

#include <chrono>

namespace jungi::mobilus_gtw_client::io {

class BlockingClientWatcher final : public ClientWatcher {
public:
    void loop();
    void loopFor(std::chrono::milliseconds duration);

    void watchTimer(TimerEventHandler* handler, std::chrono::milliseconds delay) override;
    void unwatchTimer(TimerEventHandler* handler) override;

    void watchSocket(SocketEventHandler* handler, int socketFd) override;
    void unwatchSocket(SocketEventHandler* handler, int socketFd) override;

private:
    static constexpr int kInvalidSocketFd = -1;

    SocketEventHandler* mSocketEventHandler = nullptr;
    TimerEventHandler* mTimerEventHandler = nullptr;
    int mSocketFd = kInvalidSocketFd;
    std::chrono::milliseconds mTimerDelay = std::chrono::milliseconds::max();
};

}
