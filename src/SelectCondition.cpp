#include "SelectCondition.h"

#include <sys/select.h>

namespace jungi::mobilus_gtw_client {

static constexpr uint16_t kWaitTimeSecs = 10;

SelectCondition::SelectCondition(io::SocketEventHandler& socketEventHandler, int socketFd)
    : mSocketEventHandler(socketEventHandler)
    , mSocketFd(socketFd)
{
}

void SelectCondition::wait()
{
    timeval timeout = { kWaitTimeSecs, 0 };

    fd_set readFds;
    fd_set writeFds;

    mCondition = false;

    while (!mCondition && (timeout.tv_sec > 0 || timeout.tv_usec > 0)) {
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        auto events = mSocketEventHandler.socketEvents();

        if (events.has(io::SocketEvents::Read)) {
            FD_SET(mSocketFd, &readFds);
        }
        if (events.has(io::SocketEvents::Write)) {
            FD_SET(mSocketFd, &writeFds);
        }

        if (select(mSocketFd + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            return;
        }

        io::SocketEvents revents;

        if (FD_ISSET(mSocketFd, &readFds)) {
            revents.set(io::SocketEvents::Read);
        }
        if (FD_ISSET(mSocketFd, &writeFds)) {
            revents.set(io::SocketEvents::Write);
        }

        mSocketEventHandler.handleSocketEvents(revents);
    }
}

}
