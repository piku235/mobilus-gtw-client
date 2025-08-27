#pragma once

#include "ClientId.h"
#include "ExponentialBackoff.h"
#include "SelectCondition.h"
#include "crypto/Encryptor.h"
#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClientConfig.h"
#include "jungi/mobilus_gtw_client/io/SocketEventHandler.h"
#include "jungi/mobilus_gtw_client/io/TimerEventHandler.h"

#include <mosquitto.h>

#include <cstdint>
#include <memory>
#include <queue>
#include <string>
#include <vector>

namespace jungi::mobilus_gtw_client {

namespace proto {
    class CallEvents;
}

class MqttMobilusGtwClientImpl final : public MqttMobilusGtwClient,
                                       public io::SocketEventHandler,
                                       public io::TimerEventHandler {
public:
    using Config = MqttMobilusGtwClientConfig;

    MqttMobilusGtwClientImpl(Config config);
    ~MqttMobilusGtwClientImpl();

    Result<> connect() override;
    Result<> disconnect() override;
    Result<> send(const google::protobuf::MessageLite& message) override;
    Result<> sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) override;

    MessageBus& messageBus() override { return mMessageBus; }
    const std::optional<SessionInformation>& sessionInfo() const override { return mSessionInfo; }

    // SocketEventHandler
    io::SocketEvents socketEvents() override;
    void handleSocketEvents(io::SocketEvents revents) override;

    // TimerEventHandler
    void handleTimerEvent() override;

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
    Config mConfig;
    std::queue<QueuedMessage> mMessageQueue;
    MessageBus mMessageBus;
    std::unique_ptr<crypto::Encryptor> mPublicEncryptor;
    std::unique_ptr<crypto::Encryptor> mPrivateEncryptor;
    std::optional<SessionInformation> mSessionInfo;
    ExpectedMessage* mExpectedMessage = nullptr;
    bool mConnected = false;
    bool mReconnecting = false;
    ExponentialBackoff mReconnectDelay = { std::chrono::seconds(1), std::chrono::minutes(2) };

    static void onConnectCallback(mosquitto* mosq, void* obj, int reasonCode);
    static void onMessageCallback(mosquitto* mosq, void* obj, const mosquitto_message* mosqMessage);

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
    void dispatchQueuedMessages();
    void clearSession();
    void scheduleTimer();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    std::unique_ptr<crypto::Encryptor> encryptorFor(crypto::bytes key);

    tl::unexpected<Error> logAndReturn(Error error)
    {
        mConfig.logger->error(error.message());
        return tl::unexpected(std::move(error));
    }
};

}
