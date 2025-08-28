#include "TestClientWatcher.h"
#include "TimeUtils.h"

#include <algorithm>
#include <sys/select.h>
#include <unistd.h>

using namespace jungi::mobilus_gtw_client::io;

static constexpr int kInvalidFd = -1;

namespace jungi::mobilus_gtw_client::tests::io {

void TestClientWatcher::loopFor(std::chrono::milliseconds duration)
{
    fd_set readFds;
    fd_set writeFds;

    auto timeout = TimeUtils::convertToTimeval(duration);

    while (kInvalidFd != mSocketFd && (timeout.tv_sec > 0 || timeout.tv_usec > 0)) {
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        auto socketEvents = mHandler->socketEvents();

        if (socketEvents.has(SocketEvents::Read)) {
            FD_SET(mSocketFd, &readFds);
        }
        if (socketEvents.has(SocketEvents::Write)) {
            FD_SET(mSocketFd, &writeFds);
        }

        if (select(mSocketFd + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            break;
        }

        SocketEvents revents;

        if (FD_ISSET(mSocketFd, &readFds)) {
            revents.set(SocketEvents::Read);
        }
        if (FD_ISSET(mSocketFd, &writeFds)) {
            revents.set(SocketEvents::Write);
        }

        mHandler->handleSocketEvents(revents);
    }
}

void TestClientWatcher::watchSocket(SocketEventHandler* handler, int socketFd)
{
    mHandler = handler;
    mSocketFd = socketFd;
}

void TestClientWatcher::unwatchSocket(SocketEventHandler* handler, int socketFd)
{
    mHandler = nullptr;
    mSocketFd = kInvalidFd;
}

}
