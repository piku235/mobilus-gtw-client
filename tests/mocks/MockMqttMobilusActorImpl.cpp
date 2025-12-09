#include "MockMqttMobilusActorImpl.h"
#include "crypto/EvpEncryptor.h"
#include "crypto/hash.h"
#include "crypto/utils.h"
#include "jungi/mobgtw/EventNumber.h"
#include "jungi/mobgtw/MessageType.h"
#include "jungi/mobgtw/Platform.h"
#include "jungi/mobgtw/ProtoUtils.h"
#include "jungi/mobgtw/proto/CallEvents.pb.h"
#include "jungi/mobgtw/proto/LoginRequest.pb.h"
#include "jungi/mobgtw/proto/LoginResponse.pb.h"

#include <openssl/rand.h>

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

using namespace jungi::mobgtw::io;

static const auto kHashedPassword = jungi::mobgtw::crypto::sha256(kPassword);
static const auto kEncryptor = jungi::mobgtw::crypto::EvpEncryptor::Aes256_cfb128();

template <class TContainer>
static std::string bin2hex(const TContainer& buf)
{
    std::ostringstream oss;

    for (auto it = std::begin(buf); it != std::end(buf); it++) {
        oss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << static_cast<int>(*it);
    }

    return oss.str();
}

namespace jungi::mobgtw::tests::mocks {

MockMqttMobilusActorImpl::MockMqttMobilusActorImpl(std::string host, uint16_t port)
    : mHost(std::move(host))
    , mPort(port)
{
}

MockMqttMobilusActorImpl::~MockMqttMobilusActorImpl()
{
    if (nullptr != mMosq) {
        mosquitto_disconnect(mMosq);
        mosquitto_destroy(mMosq);
        mMosq = nullptr;
    }
}

bool MockMqttMobilusActorImpl::connect()
{
    if (nullptr != mMosq) {
        return true;
    }

    int rc;
    mosquitto* mosq = mosquitto_new(kClientId, true, this);

    if (nullptr == mosq) {
        return false;
    }

    mosquitto_message_callback_set(mosq, onMessageCallback);

    rc = mosquitto_connect(mosq, mHost.c_str(), mPort, 60);

    if (MOSQ_ERR_SUCCESS != rc) {
        mosquitto_destroy(mMosq);
        return false;
    }

    rc = mosquitto_subscribe(mosq, nullptr, kCommandTopic, 0);

    if (MOSQ_ERR_SUCCESS != rc) {
        mosquitto_disconnect(mosq);
        mosquitto_destroy(mosq);

        return false;
    }

    mMosq = mosq;

    return true;
}

void MockMqttMobilusActorImpl::handle(ReplyClientCommand& cmd)
{
    auto messageType = ProtoUtils::messageTypeFor(*cmd.message);
    auto& key = MessageType::CallEvents == messageType ? mPublicKey : mPrivateKey;

    if (!mLoggedClientId.empty()) {
        send(mLoggedClientId, *cmd.message, key);
    }
}

void MockMqttMobilusActorImpl::handle(ShareMessageCommand& cmd)
{
    send(kEventsTopic, *cmd.message, mPublicKey);
}

void MockMqttMobilusActorImpl::handle(MockResponseCommand& cmd)
{
    mMockResponses.emplace(cmd.requestType, std::move(cmd.response));
}

int MockMqttMobilusActorImpl::socketFd()
{
    return mosquitto_socket(mMosq);
}

SocketEvents MockMqttMobilusActorImpl::socketEvents()
{
    if (nullptr == mMosq) {
        return {};
    }

    SocketEvents events;
    events.set(SocketEvents::Read);

    if (mosquitto_want_write(mMosq)) {
        events.set(SocketEvents::Write);
    }

    return events;
}

void MockMqttMobilusActorImpl::handleSocketEvents(SocketEvents revents)
{
    if (revents.has(SocketEvents::Read)) {
        mosquitto_loop_read(mMosq, 1);
    }
    if (revents.has(SocketEvents::Write)) {
        mosquitto_loop_write(mMosq, 1);
    }
}

bool MockMqttMobilusActorImpl::send(const std::string& topic, const google::protobuf::MessageLite& message, const crypto::bytes& key)
{
    if (nullptr == mMosq) {
        return false;
    }

    auto envelope = envelopeFor(message);

    envelope.messageBody = kEncryptor.encrypt(envelope.messageBody, key, crypto::timestamp2iv(envelope.timestamp));
    auto serialized = envelope.serialize();

    int rc = mosquitto_publish(mMosq, nullptr, topic.c_str(), serialized.size(), serialized.data(), 0, false);

    if (MOSQ_ERR_SUCCESS != rc) {
        return false;
    }

    return true;
}

void MockMqttMobilusActorImpl::onMessage(const mosquitto_message* message)
{
    auto envelope = Envelope::deserialize(reinterpret_cast<uint8_t*>(message->payload), message->payloadlen);

    if (!envelope) {
        return;
    }

    if (MessageType::LoginRequest == envelope->messageType) {
        handleLoginRequest(*envelope);
        return;
    }

    handleMockResponse(*envelope);
}

void MockMqttMobilusActorImpl::handleLoginRequest(const Envelope& envelope)
{
    proto::LoginRequest loginRequest;

    if (!loginRequest.ParseFromArray(envelope.messageBody.data(), envelope.messageBody.size())) {
        return;
    }

    auto key = crypto::bytes(loginRequest.password().begin(), loginRequest.password().end());
    auto expectedPassword = std::string(kHashedPassword.begin(), kHashedPassword.end());

    if (loginRequest.login() != kUsername || loginRequest.password() != expectedPassword) {
        proto::LoginResponse loginResponse;

        loginResponse.set_login_status(1);
        send(bin2hex(envelope.clientId), loginResponse, key);

        return;
    }

    mPublicKey = randomKey();
    mPrivateKey = randomKey();
    mLoggedClientId = bin2hex(envelope.clientId);

    proto::LoginResponse loginResponse;

    loginResponse.set_admin(true);
    loginResponse.set_login_status(0);
    loginResponse.set_user_id(1);
    loginResponse.set_serial_number(kSerialNumber);
    loginResponse.set_public_key(mPublicKey.data(), mPublicKey.size());
    loginResponse.set_private_key(mPrivateKey.data(), mPrivateKey.size());

    send(mLoggedClientId, loginResponse, key);
}

void MockMqttMobilusActorImpl::handleMockResponse(const Envelope& envelope)
{
    auto decryptedBody = kEncryptor.decrypt(envelope.messageBody, mPrivateKey, crypto::timestamp2iv(envelope.timestamp));
    auto message = ProtoUtils::newMessageFor(envelope.messageType);

    if (!message) {
        return;
    }

    if (!message->ParseFromArray(decryptedBody.data(), decryptedBody.size())) {
        return;
    }

    auto it = mMockResponses.find(envelope.messageType);
    if (it != mMockResponses.end()) {
        send(bin2hex(envelope.clientId), *it->second, mPrivateKey);
    }
}

crypto::bytes MockMqttMobilusActorImpl::randomKey()
{
    crypto::byte buf[32];

    RAND_bytes(buf, sizeof(buf));

    return { buf, buf + sizeof(buf) };
}

Envelope MockMqttMobilusActorImpl::envelopeFor(const google::protobuf::MessageLite& message)
{
    std::vector<uint8_t> messageBody;

    messageBody.resize(static_cast<size_t>(message.ByteSize()));
    message.SerializeToArray(messageBody.data(), message.ByteSize());

    return {
        ProtoUtils::messageTypeFor(message),
        static_cast<uint32_t>(time(nullptr)),
        kClientIdBinary,
        Platform::Host,
        0,
        messageBody,
    };
}

}
