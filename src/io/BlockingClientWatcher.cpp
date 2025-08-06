#include "jungi/mobilus_gtw_client/io/BlockingClientWatcher.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

#include <chrono>
#include <sys/select.h>
#include <thread>

using std::chrono::steady_clock;

static timeval convertToTimeval(std::chrono::milliseconds ms)
{
    return {
        static_cast<time_t>(ms.count() / 1000),
        static_cast<suseconds_t>((ms.count() % 1000) * 1000)
    };
}

namespace jungi::mobilus_gtw_client::io {

void BlockingClientWatcher::loop()
{
    if (nullptr == mSocketEventHandler && nullptr == mTimerEventHandler) {
        return;
    }

    auto lastTimerAt = steady_clock::now();

    fd_set readFds;
    fd_set writeFds;

    while (nullptr != mTimerEventHandler || nullptr != mSocketEventHandler) {
        if (nullptr != mTimerEventHandler && steady_clock::now() - lastTimerAt >= mTimerDelay) {
            mTimerEventHandler->handleTimerEvent();
            lastTimerAt = steady_clock::now();
        }

        if (nullptr == mSocketEventHandler) {
            // mTimerDelay should be valid since mTimerEventHandler cannot be null at this point
            std::this_thread::sleep_for(mTimerDelay);
            continue;
        }

        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        auto events = mSocketEventHandler->socketEvents();

        if (events.has(SocketEvents::Read)) {
            FD_SET(mSocketFd, &readFds);
        }
        if (events.has(SocketEvents::Write)) {
            FD_SET(mSocketFd, &writeFds);
        }

        auto timeout = convertToTimeval(mTimerDelay);

        if (select(mSocketFd + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            return;
        }

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
