#pragma once

#include "ClientId.h"
#include "ExponentialBackoff.h"
#include "SelectCondition.h"
#include "crypto/Encryptor.h"
#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MqttDsn.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/io/SocketEventHandler.h"
#include "jungi/mobilus_gtw_client/io/EventLoop.h"
#include "jungi/mobilus_gtw_client/io/NullEventLoop.h"
#include "jungi/mobilus_gtw_client/logging/Logger.h"
#include "jungi/mobilus_gtw_client/logging/NullLogger.h"

#include <mosquitto.h>

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <chrono>

namespace jungi::mobilus_gtw_client {

namespace proto {
    class CallEvents;
}

class MqttMobilusGtwClientImpl final : public MqttMobilusGtwClient,
                                       public io::SocketEventHandler {
public:
    MqttMobilusGtwClientImpl(MqttDsn dsn, std::chrono::milliseconds conenctTimeout, std::chrono::milliseconds responseTimeout, io::EventLoop& loop = io::NullEventLoop::instance(), logging::Logger& logger = logging::NullLogger::instance());
    ~MqttMobilusGtwClientImpl();

    void useKeepAliveMessage(std::unique_ptr<google::protobuf::MessageLite> message);
    void onSessionExpiring(SessionExpiringCallback callback);
    void onRawMessage(RawMessageCallback callback);

    Result<> connect() override;
    Result<> disconnect() override;
    Result<> send(const google::protobuf::MessageLite& message) override;
    Result<> sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) override;

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
        int reasonCode;
    };

    struct ExpectedMessage {
        SelectCondition& cond;
        uint8_t expectedMessageType;
        google::protobuf::MessageLite& expectedMessage;
        uint8_t responseStatus = 0xFF;
        std::optional<Error> error = std::nullopt;
    };

    mosquitto* mMosq = nullptr;
    ClientId mClientId;
    MqttDsn mDsn;
    std::chrono::milliseconds mConnectTimeout;
    std::chrono::milliseconds mResponseTimeout;
    io::EventLoop& mLoop;
    logging::Logger& mLogger;
    std::unique_ptr<google::protobuf::MessageLite> mKeepAliveMessage;
    SessionExpiringCallback mSessionExpiringCallback = [](int) {};
    RawMessageCallback mRawMessageCallback = [](const Envelope&) {};
    std::queue<QueuedMessage> mMessageQueue;
    MessageBus mMessageBus;
    std::unique_ptr<crypto::Encryptor> mPublicEncryptor;
    std::unique_ptr<crypto::Encryptor> mPrivateEncryptor;
    std::optional<SessionInformation> mSessionInfo;
    ExpectedMessage* mExpectedMessage = nullptr;
    bool mConnected = false;
    bool mReconnecting = false;
    ExponentialBackoff mReconnectDelay = { std::chrono::milliseconds(100), std::chrono::minutes(2) };

    static void onConnectCallback(mosquitto* mosq, void* obj, int reasonCode);
    static void onMessageCallback(mosquitto* mosq, void* obj, const mosquitto_message* mosqMessage);
    static void reconnectTimerCallback(void* callbackData) { reinterpret_cast<MqttMobilusGtwClientImpl*>(callbackData)->reconnect(); };
    static void miscTimerCallback(void* callbackData) { reinterpret_cast<MqttMobilusGtwClientImpl*>(callbackData)->handleMisc(); };

    int connectMqtt();
    void reconnect();
    Result<> send(const google::protobuf::MessageLite& message, int qos);
    Result<> login();
    void onMessage(const mosquitto_message* mosqMessage);
    void onGeneralMessage(const mosquitto_message* mosqMessage);
    void onExpectedMessage(ExpectedMessage& expectedMessage, const mosquitto_message* mosqMessage);
    void handleClientCallEvents(const proto::CallEvents& callEvents);
    void handleLostConnection(int rc);
    void handleInvalidSession();
    void handleMisc();
    void dispatchQueuedMessages();
    void clearSession();
    void scheduleMisc();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    std::unique_ptr<crypto::Encryptor> encryptorFor(crypto::bytes key);

    tl::unexpected<Error> logAndReturn(Error error)
    {
        mLogger.error(error.message());
        return tl::unexpected(std::move(error));
    }
};

}
