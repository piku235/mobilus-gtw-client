#pragma once

#include "jungi/mobilus_gtw_client/io/SocketEventHandler.h"

namespace jungi::mobilus_gtw_client {

class SelectCondition final {
public:
    explicit SelectCondition(io::SocketEventHandler& socketEventHandler, int socketFd);

    void notify();
    void wait();

private:
    io::SocketEventHandler& mSocketEventHandler;
    int mSocketFd;
    bool mCondition = false;
};

}
