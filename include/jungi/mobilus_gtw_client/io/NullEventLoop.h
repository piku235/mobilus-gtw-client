#pragma once

#include "EventLoop.h"

namespace jungi::mobilus_gtw_client::io {

class NullEventLoop final : public EventLoop {
public:
    static NullEventLoop& instance()
    {
        static NullEventLoop loop;

        return loop;
    }

    void startTimer(std::chrono::milliseconds, TimerCallback, void*) override {}
    void stopTimer(TimerCallback, void*) override {}
    void watchSocket(int, SocketEventHandler*) override {}
    void unwatchSocket(int) override {}

private:
    NullEventLoop() = default;
};

}
