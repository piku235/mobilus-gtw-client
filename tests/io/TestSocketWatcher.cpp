#include "TestSocketWatcher.h"

#include <algorithm>
#include <sys/select.h>
#include <unistd.h>

using namespace jungi::mobilus_gtw_client::io;

namespace jungi::mobilus_gtw_client::tests::io {

void TestSocketWatcher::loopFor(std::chrono::milliseconds duration)
{
    fd_set readFds;
    fd_set writeFds;

    if (pipe(mWakeFd) != 0) {
        return;
    }

    auto timeout = convertToTimeval(duration);

    while (isWatchingAnySockets() && (timeout.tv_sec > 0 || timeout.tv_usec > 0)) {
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        FD_SET(mWakeFd[0], &readFds);

        int maxFd = mWakeFd[0];

        for (int i = 0; i < kWatchedSocketsCount; i++) {
            if (kInvalidFd == mWatchedSockets[i].fd) {
                continue;
            }

            auto& watchedSocket = mWatchedSockets[i];
            auto socketEvents = watchedSocket.handler->socketEvents();

            if (socketEvents.has(SocketEvents::Read)) {
                FD_SET(watchedSocket.fd, &readFds);
            }
            if (socketEvents.has(SocketEvents::Write)) {
                FD_SET(watchedSocket.fd, &writeFds);
            }

            if (watchedSocket.fd > maxFd) {
                maxFd = watchedSocket.fd;
            }
        }

        if (select(maxFd + 1, &readFds, &writeFds, nullptr, &timeout) < 0) {
            break;
        }

        for (int i = 0; i < kWatchedSocketsCount; i++) {
            if (kInvalidFd == mWatchedSockets[i].fd) {
                continue;
            }

            auto& watchedSocket = mWatchedSockets[i];
            SocketEvents revents;

            if (FD_ISSET(watchedSocket.fd, &readFds)) {
                revents.set(SocketEvents::Read);
            }
            if (FD_ISSET(watchedSocket.fd, &writeFds)) {
                revents.set(SocketEvents::Write);
            }

            watchedSocket.handler->handleSocketEvents(revents);
        }

        if (FD_ISSET(mWakeFd[0], &readFds)) {
            break;
        }
    }

    close(mWakeFd[0]);
    close(mWakeFd[1]);

    mWakeFd[0] = kInvalidFd;
    mWakeFd[1] = kInvalidFd;
}

void TestSocketWatcher::watchSocket(SocketEventHandler* handler, int socketFd)
{
    for (int i = 0; i < kWatchedSocketsCount; i++) {
        if (kInvalidFd == mWatchedSockets[i].fd) {
            mWatchedSockets[i].fd = socketFd;
            mWatchedSockets[i].handler = handler;
            return;
        }
    }
}

void TestSocketWatcher::unwatchSocket(SocketEventHandler* handler, int socketFd)
{
    for (int i = 0; i < kWatchedSocketsCount; i++) {
        if (socketFd == mWatchedSockets[i].fd) {
            mWatchedSockets[i].fd = kInvalidFd;
            mWatchedSockets[i].handler = nullptr;
            return;
        }
    }
}

void TestSocketWatcher::stop()
{
    if (kInvalidFd == mWakeFd[1]) {
        return;
    }

    char byte = 1;
    (void)write(mWakeFd[1], &byte, 1);
}

timeval TestSocketWatcher::convertToTimeval(std::chrono::milliseconds ms)
{
    return {
        static_cast<time_t>(ms.count() / 1000),
        static_cast<suseconds_t>((ms.count() % 1000) * 1000)
    };
}

bool TestSocketWatcher::isWatchingAnySockets()
{
    for (int i = 0; i < kWatchedSocketsCount; i++) {
        if (kInvalidFd != mWatchedSockets[i].fd) {
            return true;
        }
    }

    return false;
}

}
