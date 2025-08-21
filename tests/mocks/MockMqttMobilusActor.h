#pragma once

#include "MockMqttMobilusService.h"
#include "io/TestSocketWatcher.h"

#include <google/protobuf/message_lite.h>

#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace jungi::mobilus_gtw_client::tests::mocks {

class MockMqttMobilusActor final {
public:
    MockMqttMobilusActor(std::string host, size_t port);
    ~MockMqttMobilusActor();

    void mockResponseFor(uint8_t requestType, std::unique_ptr<const google::protobuf::MessageLite> response);
    void run();
    void stop();

    bool isRunning() const;

private:
    std::string mHost;
    size_t mPort;
    std::thread mThread;
    MockMqttMobilusService::MockResponseMap mMockResponses;
    jungi::mobilus_gtw_client::tests::io::TestSocketWatcher mSocketWatcher;
};

}
