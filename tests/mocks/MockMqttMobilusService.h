#pragma once

#include "crypto/Encryptor.h"
#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/io/ClientWatcher.h"
#include "jungi/mobilus_gtw_client/io/SocketEventHandler.h"

#include <google/protobuf/message_lite.h>
#include <mosquitto.h>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>

static constexpr char kClientId[] = "123456789ABC";
static constexpr std::array<uint8_t, 6> kClientIdBinary = { 0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc };
static constexpr char kSerialNumber[] = "MB123456";
static constexpr char kUsername[] = "admin";
static constexpr char kPassword[] = "admin";
static constexpr char kCommandTopic[] = "module";
static constexpr char kEventsTopic[] = "clients";

namespace jungi::mobilus_gtw_client::tests::mocks {

namespace io = jungi::mobilus_gtw_client::io;
namespace crypto = jungi::mobilus_gtw_client::crypto;

class MockMqttMobilusService final : public io::SocketEventHandler {
public:
    using MockResponseMap = std::unordered_map<uint8_t, std::unique_ptr<const google::protobuf::MessageLite>>;

    MockMqttMobilusService(std::string host, size_t port, MockResponseMap mockResponses, io::ClientWatcher& clientWatcher);
    ~MockMqttMobilusService();

    bool connect();
    io::SocketEvents socketEvents() override;
    void handleSocketEvents(io::SocketEvents revents) override;

private:
    mosquitto* mMosq = nullptr;
    std::string mHost;
    size_t mPort;
    crypto::bytes mPublicKey;
    crypto::bytes mPrivateKey;
    std::unique_ptr<crypto::Encryptor> mPublicEncryptor;
    std::unique_ptr<crypto::Encryptor> mPrivateEncryptor;
    MockResponseMap mMockResponses;
    io::ClientWatcher& mClientWatcher;

    static void onMessageCallback(mosquitto* mosq, void* self, const mosquitto_message* message) { reinterpret_cast<MockMqttMobilusService*>(self)->onMessage(message); }
    void onMessage(const mosquitto_message* message);

    bool reply(const std::string& clientTopic, const google::protobuf::MessageLite& message);
    bool send(const std::string& clientTopic, const google::protobuf::MessageLite& message, const crypto::Encryptor& encryptor);
    crypto::bytes randomKey();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    void handleLoginRequest(const Envelope& envelope);
    void handleCallEvents(const Envelope& envelope);
    void handleMockResponse(const Envelope& envelope);
};

}
