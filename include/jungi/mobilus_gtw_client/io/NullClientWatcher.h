#pragma once

#include "ClientWatcher.h"

namespace jungi::mobilus_gtw_client::io {

class NullClientWatcher final : public ClientWatcher {
public:
    static NullClientWatcher& instance()
    {
        static NullClientWatcher clientWatcher;

        return clientWatcher;
    }

    void watchTimer(TimerEventHandler* handler, std::chrono::milliseconds delay) override {}
    void unwatchTimer(TimerEventHandler* handler) override {}

    void watchSocket(SocketEventHandler* handler, int socketFd) override {}
    void unwatchSocket(SocketEventHandler* handler, int socketFd) override {}
};

}
