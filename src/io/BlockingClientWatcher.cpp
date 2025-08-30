#include "jungi/mobilus_gtw_client/io/BlockingClientWatcher.h"
#include "TimeUtils.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

#include <chrono>
#include <sys/select.h>
#include <algorithm>

using std::chrono::steady_clock;

namespace jungi::mobilus_gtw_client::io {

void BlockingClientWatcher::loopFor(std::chrono::milliseconds duration)
{
    if (nullptr == mSocketEventHandler && nullptr == mTimerEventHandler) {
        return;
    }

    const auto untilTime = steady_clock::now() + duration;
    auto lastTimerAt = steady_clock::now();

    fd_set readFds;
    fd_set writeFds;

    while (steady_clock::now() <= untilTime && (nullptr != mTimerEventHandler || nullptr != mSocketEventHandler)) {
        auto absoluteDelay = std::chrono::duration_cast<std::chrono::milliseconds>(untilTime - steady_clock::now());

        if (nullptr != mTimerEventHandler && steady_clock::now() - lastTimerAt >= mTimerDelay) {
            mTimerEventHandler->handleTimerEvent();
            lastTimerAt = steady_clock::now();
        }

        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        int nfds = kInvalidSocketFd;

        if (nullptr != mSocketEventHandler) {
            auto events = mSocketEventHandler->socketEvents();

            if (events.has(SocketEvents::Read)) {
                FD_SET(mSocketFd, &readFds);
            }
            if (events.has(SocketEvents::Write)) {
                FD_SET(mSocketFd, &writeFds);
            }

            nfds = mSocketFd;
        }

        auto timeout = TimeUtils::convertToTimeval(std::min(mTimerDelay, absoluteDelay));

        if (select(nfds + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            return;
        }

        if (nullptr != mSocketEventHandler) {
            SocketEvents revents;

            if (FD_ISSET(mSocketFd, &readFds)) {
                revents.set(SocketEvents::Read);
            }
            if (FD_ISSET(mSocketFd, &writeFds)) {
                revents.set(SocketEvents::Write);
            }

            mSocketEventHandler->handleSocketEvents(revents);
        }
    }
}

void BlockingClientWatcher::loop()
{
    loopFor(std::chrono::milliseconds::max());
}

void BlockingClientWatcher::watchTimer(TimerEventHandler* handler, std::chrono::milliseconds delay)
{
    if (nullptr != mTimerEventHandler && mTimerEventHandler != handler) {
        return;
    }

    mTimerEventHandler = handler;
    mTimerDelay = delay;
}

void BlockingClientWatcher::unwatchTimer(TimerEventHandler* handler)
{
    if (mTimerEventHandler != handler) {
        return;
    }

    mTimerEventHandler = nullptr;
    mTimerDelay = std::chrono::milliseconds::max();
}

void BlockingClientWatcher::watchSocket(SocketEventHandler* handler, int socketFd)
{
    if (nullptr != mSocketEventHandler && mSocketEventHandler != handler) {
        return;
    }

    mSocketEventHandler = handler;
    mSocketFd = socketFd;
}

void BlockingClientWatcher::unwatchSocket(SocketEventHandler* handler, int)
{
    if (mSocketEventHandler != handler) {
        return;
    }

    mSocketEventHandler = nullptr;
    mSocketFd = kInvalidSocketFd;
}

}
