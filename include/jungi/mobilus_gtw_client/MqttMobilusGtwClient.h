#pragma once

#include "Error.h"
#include "MessageBus.h"
#include "MqttMobilusGtwClientConfig.h"
#include "SessionInformation.h"

#include <google/protobuf/message_lite.h>
#include <tl/expected.hpp>

#include <memory>
#include <optional>

namespace jungi::mobilus_gtw_client {

class MqttMobilusGtwClient {
public:
    template <typename TOk = void>
    using Result = tl::expected<TOk, Error>;

    static std::unique_ptr<MqttMobilusGtwClient> with(MqttMobilusGtwClientConfig config);

    virtual ~MqttMobilusGtwClient() = default;

    virtual Result<> connect() = 0;
    virtual Result<> disconnect() = 0;
    virtual Result<> send(const google::protobuf::MessageLite& message) = 0;
    virtual Result<> sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) = 0;

    virtual MessageBus& messageBus() = 0;
    virtual const std::optional<SessionInformation>& sessionInfo() const = 0;
};

}
