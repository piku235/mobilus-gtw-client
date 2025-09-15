#pragma once

#include "crypto/Encryptor.h"
#include "jungi/mobilus_gtw_client/Envelope.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/io/SocketEvents.h"

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

namespace jungi::mobilus_gtw_client::proto {

class CallEvents;

}

namespace jungi::mobilus_gtw_client::tests::mocks {

namespace proto = jungi::mobilus_gtw_client::proto;
namespace io = jungi::mobilus_gtw_client::io;
namespace crypto = jungi::mobilus_gtw_client::crypto;

class MockMqttMobilusActorImpl final {
public:
    enum class Commands {
        ReplyClient,
        ShareMessage,
        MockResponse,
    };

    struct Command {
        virtual ~Command() = default;
        virtual Commands commandId() const = 0;
    };

    struct ReplyClientCommand final : public Command {
        std::unique_ptr<const google::protobuf::MessageLite> message;

        ReplyClientCommand(std::unique_ptr<const google::protobuf::MessageLite> aMessage)
            : message(std::move(aMessage))
        {
        }

        Commands commandId() const { return Commands::ReplyClient; }
    };

    struct ShareMessageCommand final : public Command {
        std::unique_ptr<const google::protobuf::MessageLite> message;

        ShareMessageCommand(std::unique_ptr<const google::protobuf::MessageLite> aMessage)
            : message(std::move(aMessage))
        {
        }

        Commands commandId() const { return Commands::ShareMessage; }
    };

    struct MockResponseCommand final : public Command {
        uint8_t requestType;
        std::unique_ptr<const google::protobuf::MessageLite> response;

        MockResponseCommand(uint8_t aRequestType, std::unique_ptr<const google::protobuf::MessageLite> aResponse)
            : requestType(aRequestType)
            , response(std::move(aResponse))
        {
        }

        Commands commandId() const { return Commands::MockResponse; }
    };

    MockMqttMobilusActorImpl(std::string host, uint16_t port);
    ~MockMqttMobilusActorImpl();

    bool connect();
    void handle(ReplyClientCommand& cmd);
    void handle(ShareMessageCommand& cmd);
    void handle(MockResponseCommand& cmd);

    int socketFd();
    io::SocketEvents socketEvents();
    void handleSocketEvents(io::SocketEvents revents);

private:
    using MockResponseMap = std::unordered_map<uint8_t, std::unique_ptr<const google::protobuf::MessageLite>>;

    mosquitto* mMosq = nullptr;
    std::string mHost;
    uint16_t mPort;
    crypto::bytes mPublicKey;
    crypto::bytes mPrivateKey;
    std::unique_ptr<crypto::Encryptor> mPublicEncryptor;
    std::unique_ptr<crypto::Encryptor> mPrivateEncryptor;
    MockResponseMap mMockResponses;
    std::string mLoggedClientId;

    static void onMessageCallback(mosquitto* mosq, void* self, const mosquitto_message* message) { reinterpret_cast<MockMqttMobilusActorImpl*>(self)->onMessage(message); }
    void onMessage(const mosquitto_message* message);

    bool send(const std::string& topic, const google::protobuf::MessageLite& message, const crypto::Encryptor& encryptor);
    crypto::bytes randomKey();
    Envelope envelopeFor(const google::protobuf::MessageLite& message);
    void handleLoginRequest(const Envelope& envelope);
    void handleCallEvents(const Envelope& envelope);
    void handleMockResponse(const Envelope& envelope);
};

}
