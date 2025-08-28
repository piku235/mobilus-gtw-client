#pragma once

#include "jungi/mobilus_gtw_client/io/ClientWatcher.h"

#include <chrono>

namespace jungi::mobilus_gtw_client::tests::io {

class TestClientWatcher final : public jungi::mobilus_gtw_client::io::ClientWatcher {
public:
    void loopFor(std::chrono::milliseconds duration);

    // TimerEventHandler
    void watchTimer(jungi::mobilus_gtw_client::io::TimerEventHandler* handler, std::chrono::milliseconds delay) override { }
    void unwatchTimer(jungi::mobilus_gtw_client::io::TimerEventHandler* handler) override { }

    // SocketEventHandler
    void watchSocket(jungi::mobilus_gtw_client::io::SocketEventHandler* handler, int socketFd) override;
    void unwatchSocket(jungi::mobilus_gtw_client::io::SocketEventHandler* handler, int socketFd) override;

private:
    int mSocketFd = -1;
    jungi::mobilus_gtw_client::io::SocketEventHandler* mHandler = nullptr;
};

}
