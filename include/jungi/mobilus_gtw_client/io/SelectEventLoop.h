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
    void runFor(std::chrono::milliseconds duration);
    void stop();

    TimerId startTimer(std::chrono::milliseconds delay, TimerCallback callback, void* callbackData) override;
    void stopTimer(TimerId id) override;
    void watchSocket(int socketFd, SocketEventHandler* handler) override;
    void unwatchSocket(int socketFd) override;

private:
    static constexpr int kSocketWatchCount = 32;
    static constexpr int kTimerCount = 32;
    static constexpr int kInvalidFd = -1;

    struct Timer {
        std::chrono::steady_clock::time_point expiresAt = std::chrono::steady_clock::time_point::max();
        TimerCallback callback = nullptr;
        void* callbackData;

        bool isActive() const { return nullptr != callback; }
        bool hasExpired() const { return expiresAt <= std::chrono::steady_clock::now(); }
    };

    struct SocketWatch {
        int socketFd = kInvalidFd;
        SocketEventHandler* handler = nullptr;

        bool isWatching() const { return kInvalidFd != socketFd; }
    };

    Timer mTimers[kTimerCount];
    SocketWatch mSocketWatches[kSocketWatchCount];
    bool mRun = true;
};

}
