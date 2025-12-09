#pragma once

#include "ClientId.h"
#include "ExponentialBackoff.h"
#include "SelectCondition.h"
#include "jungi/mobgtw/Envelope.h"
#include "jungi/mobgtw/MobilusCredentials.h"
#include "jungi/mobgtw/MqttDsn.h"
#include "jungi/mobgtw/MqttMobilusGtwClient.h"
#include "jungi/mobgtw/io/EventLoop.h"
#include "jungi/mobgtw/io/NullEventLoop.h"
#include "jungi/mobgtw/io/SocketEventHandler.h"
#include "jungi/mobgtw/logging/Logger.h"
#include "jungi/mobgtw/logging/NullLogger.h"

#include <mosquitto.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace jungi::mobgtw {

namespace proto {
    class CallEvents;
}

class MqttMobilusGtwClientImpl final : public MqttMobilusGtwClient,
                                       public io::SocketEventHandler {
public:
    MqttMobilusGtwClientImpl(MqttDsn dsn, MobilusCredentials mobilusCreds, std::chrono::milliseconds conenctTimeout, std::chrono::milliseconds responseTimeout, io::EventLoop& loop = io::NullEventLoop::instance(), logging::Logger& logger = logging::NullLogger::instance());
    ~MqttMobilusGtwClientImpl();

    void useKeepAliveMessage(std::unique_ptr<google::protobuf::MessageLite> message);
    void onSessionExpiring(SessionExpiringCallback callback);
    void onRawMessage(RawMessageCallback callback);

    Result<> connect() override;
    Result<> disconnect() override;
    Result<> send(const google::protobuf::MessageLite& message) override;
    Result<> sendRequest(const proto::CurrentStateRequest& request, proto::CurrentStateResponse& response) override;
    Result<> sendRequest(const proto::DeviceSettingsRequest& request, proto::DeviceSettingsResponse& response) override;
    Result<> sendRequest(const proto::DevicesListRequest& request, proto::DevicesListResponse& response) override;
    Result<> sendRequest(const proto::NetworkSettingsRequest& request, proto::NetworkSettingsResponse& response) override;
    Result<> sendRequest(const proto::UpdateDeviceRequest& request, proto::UpdateDeviceResponse& response) override;

    MessageBus& messageBus() override { return mMessageBus; }
    const std::optional<SessionInformation>& sessionInfo() const override { return mSessionInfo; }

    // SocketEventHandler
    io::SocketEvents socketEvents() override;
    void handleSocketEvents(io::SocketEvents revents) override;

private:
    struct QueuedMessage {
        std::string topic;
        uint8_t messageType;
        std::unique_ptr<google::protobuf::MessageLite> message;
    };

    class LibInit {
    public:
        LibInit();
        ~LibInit();
    };

    struct ConnectCallbackContext {
        SelectCondition& cond;
        int returnCode;
    };

    struct ExpectedResponse {
        SelectCondition& cond;
        uint8_t messageType;
        google::protobuf::MessageLite& message;
        const std::vector<uint8_t>& key;
        std::optional<Error> error = std::nullopt;
    };

    mosquitto* mMosq = nullptr;
    ClientId mClientId;
    MqttDsn mDsn;
    MobilusCredentials mMobilusCreds;
    std::chrono::milliseconds mConnectTimeout;
    std::chrono::milliseconds mResponseTimeout;
    io::EventLoop& mLoop;
    logging::Logger& mLogger;
    std::unique_ptr<google::protobuf::MessageLite> mKeepAliveMessage;
    SessionExpiringCallback mSessionExpiringCallback = [](int) { };
    RawMessageCallback mRawMessageCallback = [](const Envelope&) { };
    std::queue<QueuedMessage> mMessageQueue;
    MessageBus mMessageBus;
    std::optional<SessionInformation> mSessionInfo;
    ExpectedResponse* mExpectedResponse = nullptr;
    bool mConnected = false;
    bool mReconnecting = false;
    ExponentialBackoff mReconnectDelay = { std::chrono::milliseconds(100), std::chrono::minutes(2) };
    io::EventLoop::TimerId mReconnectTimerId = io::EventLoop::kInvalidTimerId;
    io::EventLoop::TimerId mMiscTimerId = io::EventLoop::kInvalidTimerId;

    static void onConnectCallback(mosquitto* mosq, void* obj, int reasonCode);
    static void onMessageCallback(mosquitto* mosq, void* obj, const mosquitto_message* mosqMessage);
    static void reconnectTimerCallback(void* callbackData) { reinterpret_cast<MqttMobilusGtwClientImpl*>(callbackData)->reconnect(); };
    static void miscTimerCallback(void* callbackData) { reinterpret_cast<MqttMobilusGtwClientImpl*>(callbackData)->handleMisc(); };

    int connectMqtt();
    void reconnect();
    Result<> send(const google::protobuf::MessageLite& message, int qos);
    Result<> sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response, const std::vector<uint8_t>& key);
    Result<> sendSessionRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response);
    Result<> login();
    void onMessage(const mosquitto_message* mosqMessage);
    void onGeneralMessage(const mosquitto_message* mosqMessage);
    void onExpectedResponse(ExpectedResponse& expectedResponse, const mosquitto_message* mosqMessage);
    void handleClientCallEvents(const proto::CallEvents& callEvents);
    void handleLostConnection(int rc);
    void handleInvalidSession();
    void handleMisc();
    void dispatchQueuedMessages();
    void scheduleMisc();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    std::string explainConnackCode(int code);

    tl::unexpected<Error> logAndReturn(Error error)
    {
        mLogger.error(error.message());
        return tl::unexpected(std::move(error));
    }
};

}
