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

    TimerId startTimer(std::chrono::milliseconds, TimerCallback, void*) override { return kInvalidTimerId; }
    void stopTimer(TimerId) override { }
    void watchSocket(int, SocketEventHandler*) override { }
    void unwatchSocket(int) override { }

private:
    NullEventLoop() = default;
};

}
