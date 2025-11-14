#include "jungi/mobilus_gtw_client/io/SelectEventLoop.h"
#include "TimeUtils.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

#include <algorithm>
#include <sys/select.h>

namespace jungi::mobilus_gtw_client::io {

void SelectEventLoop::runUntil(Clock::time_point time)
{
    fd_set readFds;
    fd_set writeFds;

    while (mRun && Clock::now() <= time) {
        bool activeTimers = false;

        for (auto& timer : mTimers) {
            if (!timer.isActive()) {
                continue;
            }

            activeTimers = true;

            if (timer.hasExpired()) {
                auto callback = timer.callback;
                auto callbackData = timer.callbackData;

                // release slot and then call callback
                timer = {};
                callback(callbackData);
            }
        }

        // consider new timers as well
        auto nextExpiration = Clock::time_point::max();
        for (auto& timer : mTimers) {
            if (timer.isActive() && timer.expiresAt < nextExpiration) {
                nextExpiration = timer.expiresAt;
            }
        }

        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        int nfds = kInvalidFd;

        for (auto& socketWatch : mSocketWatches) {
            if (!socketWatch.isWatching()) {
                continue;
            }

            auto events = socketWatch.handler->socketEvents();

            if (events.has(SocketEvents::Read)) {
                FD_SET(socketWatch.socketFd, &readFds);
            }
            if (events.has(SocketEvents::Write)) {
                FD_SET(socketWatch.socketFd, &writeFds);
            }

            if (socketWatch.socketFd > nfds) {
                nfds = socketWatch.socketFd;
            }
        }

        // break the loop
        if (nfds == kInvalidFd && !activeTimers) {
            break;
        }

        auto now = Clock::now();
        auto timerDelay = std::min(time - now, nextExpiration - now);
        auto timeout = TimeUtils::convertToTimeval(timerDelay);

        if (select(nfds + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            break;
        }

        for (auto& socketWatch : mSocketWatches) {
            if (!socketWatch.isWatching()) {
                continue;
            }

            SocketEvents revents;

            if (FD_ISSET(socketWatch.socketFd, &readFds)) {
                revents.set(SocketEvents::Read);
            }
            if (FD_ISSET(socketWatch.socketFd, &writeFds)) {
                revents.set(SocketEvents::Write);
            }

            socketWatch.handler->handleSocketEvents(revents);
        }
    }
}

void SelectEventLoop::run()
{
    runUntil(Clock::time_point::max());
}

void SelectEventLoop::stop()
{
    mRun = false;
}

EventLoop::TimerId SelectEventLoop::startTimer(std::chrono::milliseconds delay, TimerCallback callback, void* callbackData)
{
    for (TimerId i = 0; i < kTimerCount; i++) {
        auto& timer = mTimers[i];
        if (nullptr != timer.callback) {
            continue;
        }

        timer.expiresAt = Clock::now() + delay;
        timer.callback = callback;
        timer.callbackData = callbackData;

        return i;
    }

    return kInvalidTimerId;
}

void SelectEventLoop::stopTimer(TimerId id)
{
    if (id > kInvalidTimerId && id < kTimerCount) {
        mTimers[id] = {};
    }
}

void SelectEventLoop::watchSocket(int socketFd, SocketEventHandler* handler)
{
    unwatchSocket(socketFd);

    for (auto& socketWatch : mSocketWatches) {
        if (kInvalidFd == socketWatch.socketFd) {
            socketWatch.socketFd = socketFd;
            socketWatch.handler = handler;
            return;
        }
    }
}

void SelectEventLoop::unwatchSocket(int socketFd)
{
    for (auto& socketWatch : mSocketWatches) {
        if (socketFd == socketWatch.socketFd) {
            socketWatch = {};
            return;
        }
    }
}

}
