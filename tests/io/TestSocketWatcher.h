#pragma once

#include "jungi/mobilus_gtw_client/io/ClientWatcher.h"

#include <chrono>
#include <sys/time.h>

static constexpr int kInvalidFd = -1;
static constexpr int kWatchedSocketsCount = 16;

namespace jungi::mobilus_gtw_client::tests::io {

class TestSocketWatcher final : public jungi::mobilus_gtw_client::io::ClientWatcher {
public:
    void loopFor(std::chrono::milliseconds duration);
    void stop();

    // TimerEventHandler
    void watchTimer(jungi::mobilus_gtw_client::io::TimerEventHandler* handler, std::chrono::milliseconds delay) override { }
    void unwatchTimer(jungi::mobilus_gtw_client::io::TimerEventHandler* handler) override { }

    // SocketEventHandler
    void watchSocket(jungi::mobilus_gtw_client::io::SocketEventHandler* handler, int socketFd) override;
    void unwatchSocket(jungi::mobilus_gtw_client::io::SocketEventHandler* handler, int socketFd) override;

private:
    struct WatchedSocket {
        int fd = kInvalidFd;
        jungi::mobilus_gtw_client::io::SocketEventHandler* handler;
    };

    WatchedSocket mWatchedSockets[kWatchedSocketsCount];
    int mWakeFd[2] = { kInvalidFd, kInvalidFd };

    timeval convertToTimeval(std::chrono::milliseconds ms);
    bool isWatchingAnySockets();
};

}
