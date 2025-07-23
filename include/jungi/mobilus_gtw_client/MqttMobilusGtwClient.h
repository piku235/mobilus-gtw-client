#pragma once

#include "MessageBus.h"
#include "MqttMobilusGtwClientConfig.h"
#include "SessionInformation.h"

#include <google/protobuf/message_lite.h>

#include <memory>
#include <optional>

namespace jungi::mobilus_gtw_client {

class MqttMobilusGtwClient {
public:
    static std::unique_ptr<MqttMobilusGtwClient> with(MqttMobilusGtwClientConfig config);

    virtual ~MqttMobilusGtwClient() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool send(const google::protobuf::MessageLite& message) = 0;
    virtual bool sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) = 0;

    virtual MessageBus& messageBus() = 0;
    virtual const std::optional<SessionInformation>& sessionInfo() const = 0;
};

}
