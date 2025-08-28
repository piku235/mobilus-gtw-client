#include "MockMqttMobilusActor.h"

#include <cstdint>
#include <algorithm>
#include <sys/select.h>
#include <unistd.h>

using namespace jungi::mobilus_gtw_client;

static constexpr int kInvalidFd = -1;

namespace jungi::mobilus_gtw_client::tests::mocks {

MockMqttMobilusActor::MockMqttMobilusActor(std::string host, size_t port)
{
    mSelf = std::thread([host = std::move(host), port, this]() {
        Impl impl(std::move(host), port);
        run(impl);
    });

    std::unique_lock<std::mutex> lock(mMutex);
    mCv.wait(lock, [&]() -> bool { return mReady; });
}

MockMqttMobilusActor::~MockMqttMobilusActor()
{
    stop();
}

void MockMqttMobilusActor::mockResponseFor(uint8_t requestType, std::unique_ptr<const google::protobuf::MessageLite> response)
{
    post(std::make_unique<Impl::MockResponseCommand>(requestType, std::move(response)));
}

void MockMqttMobilusActor::reply(std::unique_ptr<const google::protobuf::MessageLite> message)
{
    post(std::make_unique<Impl::ReplyClientCommand>(std::move(message)));
}

void MockMqttMobilusActor::share(std::unique_ptr<const google::protobuf::MessageLite> message)
{
    post(std::make_unique<Impl::ShareMessageCommand>(std::move(message)));
}

void MockMqttMobilusActor::run(Impl& impl)
{
    {
        std::unique_lock<std::mutex> lock(mMutex);

        auto r = impl.connect();

        mReady = true;
        lock.unlock();
        mCv.notify_one();

        if (!r) {
            return;
        }
    }

    if (pipe(mWakeFd) != 0) {
        return;
    }

    fd_set readFds;
    fd_set writeFds;

    while (!mStop && kInvalidFd != impl.socketFd()) {
        const int socketFd = impl.socketFd();

        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        int maxFd = std::max(mWakeFd[0], socketFd);
        auto events = impl.socketEvents();

        if (events.has(io::SocketEvents::Read)) {
            FD_SET(socketFd, &readFds);
        }
        if (events.has(io::SocketEvents::Write)) {
            FD_SET(socketFd, &writeFds);
        }

        FD_SET(mWakeFd[0], &readFds);

        if (select(maxFd + 1, &readFds, &writeFds, nullptr, nullptr) < 0) {
            break;
        }

        io::SocketEvents revents;

        if (FD_ISSET(socketFd, &readFds)) {
            revents.set(io::SocketEvents::Read);
        }
        if (FD_ISSET(socketFd, &writeFds)) {
            revents.set(io::SocketEvents::Write);
        }

        impl.handleSocketEvents(revents);

        if (FD_ISSET(mWakeFd[0], &readFds)) {
            consumeWakeUp();
        }

        processQueue(impl);
    }

    close(mWakeFd[0]);
    close(mWakeFd[1]);

    mWakeFd[0] = kInvalidFd;
    mWakeFd[1] = kInvalidFd;
}

void MockMqttMobilusActor::stop()
{
    mStop = true;
    wakeUp();

    if (mSelf.joinable()) {
        mSelf.join();
    }
}

void MockMqttMobilusActor::post(std::unique_ptr<Impl::Command> cmd)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.push(std::move(cmd));
    wakeUp();
}

void MockMqttMobilusActor::wakeUp()
{
    if (kInvalidFd == mWakeFd[1]) {
        return;
    }

    uint8_t byte = 1;
    ::write(mWakeFd[1], &byte, 1);
}

void MockMqttMobilusActor::consumeWakeUp()
{
    if (kInvalidFd == mWakeFd[0]) {
        return;
    }

    uint8_t buf[64];
    ::read(mWakeFd[0], &buf, sizeof(buf));
}

void MockMqttMobilusActor::processQueue(Impl& impl)
{
    std::lock_guard<std::mutex> lock(mMutex);

    while (!mQueue.empty()) {
        auto& cmd = mQueue.front();

        switch (cmd->commandId()) {
        case Impl::Commands::ReplyClient:
            impl.handle(static_cast<Impl::ReplyClientCommand&>(*cmd));
            break;
        case Impl::Commands::ShareMessage:
            impl.handle(static_cast<Impl::ShareMessageCommand&>(*cmd));
            break;
        case Impl::Commands::MockResponse:
            impl.handle(static_cast<Impl::MockResponseCommand&>(*cmd));
            break;
        }

        mQueue.pop();
    }
}

}
