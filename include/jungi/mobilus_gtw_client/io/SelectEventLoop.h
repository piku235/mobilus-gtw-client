#pragma once

#include "EventLoop.h"
#include "SocketEventHandler.h"

#include <chrono>

namespace jungi::mobilus_gtw_client::io {

class SelectEventLoop final : public EventLoop {
public:
    SelectEventLoop() = default;

    SelectEventLoop(SelectEventLoop&& other) = delete;
    SelectEventLoop& operator=(SelectEventLoop&& other) = delete;

    SelectEventLoop(const SelectEventLoop& other) = delete;
    SelectEventLoop& operator=(const SelectEventLoop& other) = delete;

    void run();

    template <typename R, typename P>
    void runFor(std::chrono::duration<R, P> duration)
    {
        runUntil(Clock::now() + duration);
    }

    template <typename C, typename D>
    void runFor(std::chrono::time_point<C, D> time)
    {
        runUntil(std::chrono::time_point_cast<Clock::duration>(time));
    }

    void stop();

    TimerId startTimer(std::chrono::milliseconds delay, TimerCallback callback, void* callbackData) override;
    void stopTimer(TimerId id) override;
    void watchSocket(int socketFd, SocketEventHandler* handler) override;
    void unwatchSocket(int socketFd) override;

private:
    using Clock = std::chrono::steady_clock;

    static constexpr int kSocketWatchCount = 32;
    static constexpr int kTimerCount = 64;
    static constexpr int kInvalidFd = -1;

    struct Timer {
        Clock::time_point expiresAt;
        TimerCallback callback = nullptr;
        void* callbackData;

        bool isActive() const { return nullptr != callback; }
        bool hasExpired() const { return expiresAt <= Clock::now(); }
    };

    struct SocketWatch {
        int socketFd = kInvalidFd;
        SocketEventHandler* handler = nullptr;

        bool isWatching() const { return kInvalidFd != socketFd; }
    };

    Timer mTimers[kTimerCount];
    SocketWatch mSocketWatches[kSocketWatchCount];
    bool mRun = true;

    void runUntil(Clock::time_point time);
};

}
