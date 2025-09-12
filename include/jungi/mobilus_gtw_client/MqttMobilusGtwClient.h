#pragma once

#include "Error.h"
#include "Envelope.h"
#include "MqttDsn.h"
#include "MessageBus.h"
#include "SessionInformation.h"
#include "io/EventLoop.h"
#include "io/NullEventLoop.h"
#include "logging/Logger.h"
#include "logging/NullLogger.h"

#include <google/protobuf/message_lite.h>
#include <tl/expected.hpp>

#include <chrono>
#include <memory>
#include <optional>
#include <functional>

namespace jungi::mobilus_gtw_client {

class MqttMobilusGtwClient {
public:
    template <typename TOk = void>
    using Result = tl::expected<TOk, Error>;

    using RawMessageCallback = std::function<void(const Envelope&)>;
    using SessionExpiringCallback = std::function<void(int remaningTime)>;

    class Builder final {
    public:
        Builder& dsn(MqttDsn dsn);
        Builder& attachTo(io::EventLoop* loop);
        Builder& useLogger(logging::Logger* logger);
        Builder& useKeepAliveMessage(std::unique_ptr<google::protobuf::MessageLite> message);
        Builder& connectTimeout(std::chrono::milliseconds timeout);
        Builder& responseTimeout(std::chrono::milliseconds timeout);
        Builder& onSessionExpiring(SessionExpiringCallback callback);
        Builder& onRawMessage(RawMessageCallback callback);
        std::unique_ptr<MqttMobilusGtwClient> build();

    private:
        std::optional<MqttDsn> mDsn;
        io::EventLoop* mLoop = &io::NullEventLoop::instance();
        logging::Logger* mLogger = &logging::NullLogger::instance();
        std::unique_ptr<google::protobuf::MessageLite> mKeepAliveMessage;
        std::chrono::milliseconds mConnectTimeout = std::chrono::seconds(1);
        std::chrono::milliseconds mResponseTimeout = std::chrono::seconds(5);
        std::optional<SessionExpiringCallback> mSessionExpiringCallback;
        std::optional<RawMessageCallback> mRawMessageCallback;
    };

    static std::unique_ptr<MqttMobilusGtwClient> fromDsn(MqttDsn dsn);
    static Builder builder();

    virtual ~MqttMobilusGtwClient() = default;

    virtual Result<> connect() = 0;
    virtual Result<> disconnect() = 0;
    virtual Result<> send(const google::protobuf::MessageLite& message) = 0;
    virtual Result<> sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) = 0;

    virtual MessageBus& messageBus() = 0;
    virtual const std::optional<SessionInformation>& sessionInfo() const = 0;
};

}
