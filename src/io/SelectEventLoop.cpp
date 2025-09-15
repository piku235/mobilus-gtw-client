#include "jungi/mobilus_gtw_client/io/SelectEventLoop.h"
#include "TimeUtils.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

#include <algorithm>
#include <sys/select.h>

using std::chrono::steady_clock;

namespace jungi::mobilus_gtw_client::io {

void SelectEventLoop::runFor(std::chrono::milliseconds duration)
{
    const auto untilTime = std::chrono::milliseconds::max() == duration ? steady_clock::time_point::max() : steady_clock::now() + duration;

    fd_set readFds;
    fd_set writeFds;

    while (mRun && steady_clock::now() <= untilTime) {
        auto absoluteDelay = std::chrono::duration_cast<std::chrono::milliseconds>(untilTime - steady_clock::now());
        auto* closestTimerExpiresAt = &mTimers[0].expiresAt;
        bool hasActiveTimers = false;

        for (auto& timer : mTimers) {
            if (!timer.isActive()) {
                continue;
            }

            hasActiveTimers = true;

            if (!timer.hasExpired()) {
                if (timer.expiresAt < *closestTimerExpiresAt) {
                    closestTimerExpiresAt = &timer.expiresAt;
                }

                continue;
            }

            auto callback = timer.callback;
            auto callbackData = timer.callbackData;

            // release slot and then call callback
            timer = {};
            callback(callbackData);
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
        if (nfds == kInvalidFd && !hasActiveTimers) {
            break;
        }

        auto timerDelay = std::chrono::duration_cast<std::chrono::milliseconds>(*closestTimerExpiresAt - steady_clock::now());
        auto timeout = TimeUtils::convertToTimeval(std::min(timerDelay, absoluteDelay));

        if (select(nfds + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            return;
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
    runFor(std::chrono::milliseconds::max());
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

        timer.expiresAt = steady_clock::now() + delay;
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
