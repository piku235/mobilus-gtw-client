#pragma once

#include "jungi/mobilus_gtw_client/io/SocketEventHandler.h"

#include <cstdint>

namespace jungi::mobilus_gtw_client {

class SelectCondition final {
public:
    SelectCondition(io::SocketEventHandler& socketEventHandler, int socketFd, uint32_t timeoutMs);

    void wait();
    void notify() { mCondition = true; }
    bool condition() const { return mCondition; }

private:
    io::SocketEventHandler& mSocketEventHandler;
    int mSocketFd;
    uint32_t mTimeoutMs;
    bool mCondition = false;
};

}
