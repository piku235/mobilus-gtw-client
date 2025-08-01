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

    void watchTimer(TimerEventHandler*, std::chrono::milliseconds) override {}
    void unwatchTimer(TimerEventHandler*) override {}

    void watchSocket(SocketEventHandler*, int) override {}
    void unwatchSocket(SocketEventHandler*, int) override {}
};

}
