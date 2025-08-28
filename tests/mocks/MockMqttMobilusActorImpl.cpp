#include "MockMqttMobilusActorImpl.h"
#include "crypto/Aes256Encryptor.h"
#include "crypto/hash.h"
#include "crypto/utils.h"
#include "jungi/mobilus_gtw_client/EventNumber.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/Platform.h"
#include "jungi/mobilus_gtw_client/ProtoUtils.h"
#include "jungi/mobilus_gtw_client/proto/CallEvents.pb.h"
#include "jungi/mobilus_gtw_client/proto/LoginRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/LoginResponse.pb.h"

#include <openssl/rand.h>

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

using namespace jungi::mobilus_gtw_client::io;

static const auto kHashedPassword = jungi::mobilus_gtw_client::crypto::sha256(kPassword);

template <class TContainer>
static std::string bin2hex(const TContainer& buf)
{
    std::ostringstream oss;

    for (auto it = std::begin(buf); it != std::end(buf); it++) {
        oss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << static_cast<int>(*it);
    }

    return oss.str();
}

namespace jungi::mobilus_gtw_client::tests::mocks {

MockMqttMobilusActorImpl::MockMqttMobilusActorImpl(std::string host, size_t port)
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
    auto& encryptor = MessageType::CallEvents == messageType ? mPublicEncryptor : mPrivateEncryptor;

    if (!mLoggedClientId.empty() && encryptor) {
        send(mLoggedClientId, *cmd.message, *encryptor);
    }
}

void MockMqttMobilusActorImpl::handle(ShareMessageCommand& cmd)
{
    if (mPublicEncryptor) {
        send(kEventsTopic, *cmd.message, *mPublicEncryptor);
    }
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

bool MockMqttMobilusActorImpl::send(const std::string& topic, const google::protobuf::MessageLite& message, const crypto::Encryptor& encryptor)
{
    if (nullptr == mMosq) {
        return false;
    }

    auto envelope = envelopeFor(message);

    envelope.messageBody = encryptor.encrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));
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

    crypto::Aes256Encryptor encryptor(std::vector<uint8_t>(loginRequest.password().begin(), loginRequest.password().end()));
    auto expectedPassword = std::string(kHashedPassword.begin(), kHashedPassword.end());

    if (loginRequest.login() != kUsername || loginRequest.password() != expectedPassword) {
        proto::LoginResponse loginResponse;

        loginResponse.set_login_status(1);
        send(bin2hex(envelope.clientId), loginResponse, encryptor);

        return;
    }

    mPublicKey = randomKey();
    mPrivateKey = randomKey();
    mPublicEncryptor = std::make_unique<crypto::Aes256Encryptor>(mPublicKey);
    mPrivateEncryptor = std::make_unique<crypto::Aes256Encryptor>(mPrivateKey);
    mLoggedClientId = bin2hex(envelope.clientId);

    proto::LoginResponse loginResponse;

    loginResponse.set_admin(true);
    loginResponse.set_login_status(0);
    loginResponse.set_user_id(1);
    loginResponse.set_serial_number(kSerialNumber);
    loginResponse.set_public_key(mPublicKey.data(), mPublicKey.size());
    loginResponse.set_private_key(mPrivateKey.data(), mPrivateKey.size());

    send(mLoggedClientId, loginResponse, encryptor);
}

void MockMqttMobilusActorImpl::handleMockResponse(const Envelope& envelope)
{
    if (!mPrivateEncryptor) {
        return;
    }

    auto decryptedBody = mPrivateEncryptor->decrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));
    auto message = ProtoUtils::newMessageFor(envelope.messageType);

    if (!message) {
        return;
    }

    if (!message->ParseFromArray(decryptedBody.data(), decryptedBody.size())) {
        return;
    }

    auto it = mMockResponses.find(envelope.messageType);
    if (it != mMockResponses.end()) {
        send(bin2hex(envelope.clientId), *it->second, *mPrivateEncryptor);
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
