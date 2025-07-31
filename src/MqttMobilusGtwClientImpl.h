#pragma once

#include "ExponentialBackoff.h"
#include "MosquittoCondition.h"
#include "crypto/Encryptor.h"
#include "jungi/mobilus_gtw_client/ClientId.h"
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
#include <tuple>
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

    bool connect() override;
    void disconnect() override;
    bool send(const google::protobuf::MessageLite& message) override;
    bool sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response) override;
    void loop();

    MessageBus& messageBus() override { return mMessageBus; }
    const std::optional<SessionInformation>& sessionInfo() const override { return mSessionInfo; }

    // SocketEventHandler
    io::SocketEvents socketEvents() override;
    void handleSocketEvents(io::SocketEvents revents) override;

    // TimerEventHandler
    void handleTimerEvent() override;

private:
    using QueuedMessage = std::tuple<std::string, Envelope, std::unique_ptr<google::protobuf::MessageLite>>;

    class LibInit {
    public:
        LibInit();
        ~LibInit();
    };

    struct ConnectCallbackContext {
        MosquittoCondition& cond;
        int reasonCode;
    };

    struct ExpectedMessage {
        MosquittoCondition& cond;
        uint8_t expectedMessageType;
        google::protobuf::MessageLite& expectedMessage;
        uint8_t responseStatus = 0xFF;
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
    std::chrono::steady_clock::time_point mLastActivity;
    bool mConnected = false;
    bool mReconnecting = false;
    ExponentialBackoff mReconnectDelay = { std::chrono::seconds(1), std::chrono::minutes(2) };

    static void onConnectCallback(mosquitto* mosq, void* obj, int reasonCode);
    static void onMessageCallback(mosquitto* mosq, void* obj, const mosquitto_message* mosqMessage);

    int connectMqtt();
    void reconnect();
    bool login();
    void onMessage(const mosquitto_message* mosqMessage);
    void onGeneralMessage(const mosquitto_message* mosqMessage);
    void onExpectedMessage(ExpectedMessage& expectedMessage, const mosquitto_message* mosqMessage);
    void handleClientCallEvents(const proto::CallEvents& callEvents);
    void handleBadMessage(const Envelope& envelope);
    void handleLostConnection();
    void processQueuedMessages();
    void clearSession();
    void bumpLastActivity();
    std::chrono::milliseconds keepAliveTimerDelay();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    std::unique_ptr<crypto::Encryptor> encryptorFor(crypto::bytes key);
};

}
