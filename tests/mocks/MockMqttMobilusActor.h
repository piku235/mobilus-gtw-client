#pragma once

#include "MockMqttMobilusActorImpl.h"

#include <google/protobuf/message_lite.h>

#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace jungi::mobgtw::tests::mocks {

class MockMqttMobilusActor final {
public:
    MockMqttMobilusActor(std::string host, uint16_t port);
    ~MockMqttMobilusActor();

    void mockResponseFor(uint8_t requestType, std::unique_ptr<const google::protobuf::MessageLite> response);
    void reply(std::unique_ptr<const google::protobuf::MessageLite> message);
    void share(std::unique_ptr<const google::protobuf::MessageLite> message);
    void start();
    void stop();

private:
    using Impl = MockMqttMobilusActorImpl;

    std::thread mSelf;
    std::string mHost;
    uint16_t mPort;
    std::mutex mMutex;
    std::promise<void> mReady;
    std::queue<std::unique_ptr<Impl::Command>> mQueue;
    bool mStop = false;
    int mWakeFd[2] = { -1, -1 };

    void run();
    void wakeUp();
    void consumeWakeUp();
    void post(std::unique_ptr<Impl::Command> cmd);
    void processQueue(Impl& impl);
};

}
