#pragma once

#include "MockMqttMobilusActorImpl.h"

#include <google/protobuf/message_lite.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace jungi::mobilus_gtw_client::tests::mocks {

class MockMqttMobilusActor final {
public:
    MockMqttMobilusActor(std::string host, size_t port);
    ~MockMqttMobilusActor();

    void mockResponseFor(uint8_t requestType, std::unique_ptr<const google::protobuf::MessageLite> response);
    void reply(std::unique_ptr<const google::protobuf::MessageLite> message);
    void share(std::unique_ptr<const google::protobuf::MessageLite> message);
    void stop();

private:
    using Impl = MockMqttMobilusActorImpl;

    std::thread mSelf;
    std::mutex mMutex;
    std::condition_variable mCv;
    std::queue<std::unique_ptr<Impl::Command>> mQueue;
    bool mReady = false;
    bool mStop = false;
    int mWakeFd[2] = { -1, -1 };

    void run(Impl& impl);
    void wakeUp();
    void consumeWakeUp();
    void post(std::unique_ptr<Impl::Command> cmd);
    void processQueue(Impl& impl);
};

}
