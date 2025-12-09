#pragma once

#include "jungi/mobgtw/io/SocketEventHandler.h"

#include <chrono>

namespace jungi::mobgtw {

class SelectCondition final {
public:
    SelectCondition(io::SocketEventHandler& socketEventHandler, int socketFd, std::chrono::milliseconds timeout);

    void wait();
    void notify() { mCondition = true; }
    bool condition() const { return mCondition; }

private:
    io::SocketEventHandler& mSocketEventHandler;
    int mSocketFd;
    std::chrono::milliseconds mTimeout;
    bool mCondition = false;
};

}
