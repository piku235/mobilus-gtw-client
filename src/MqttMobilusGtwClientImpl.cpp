#include "MqttMobilusGtwClientImpl.h"
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

#include <cerrno>
#include <ctime>
#include <utility>

static const char kEventsTopic[] = "clients";
static const char kRequestsTopic[] = "module";
static constexpr int kKeepAliveIntervalSecs = 60;

using std::chrono::steady_clock;

namespace jungi::mobilus_gtw_client {

MqttMobilusGtwClientImpl::MqttMobilusGtwClientImpl(Config config)
    : mClientId(ClientId::unique())
    , mConfig(std::move(config))
{
    static LibInit libInit;
}

MqttMobilusGtwClientImpl::~MqttMobilusGtwClientImpl()
{
    disconnect();

    if (nullptr != mMosq) {
        mosquitto_destroy(mMosq);
        mMosq = nullptr;
    }
}

bool MqttMobilusGtwClientImpl::connect()
{
    if (mConnected) {
        return true;
    }

    if (MOSQ_ERR_SUCCESS != connectMqtt()) {
        return false;
    }

    mConnected = true;
    mConfig.clientWatcher->watchSocket(this, mosquitto_socket(mMosq));

    SelectCondition cond(*this, mosquitto_socket(mMosq));
    ConnectCallbackContext connCallbackData = { cond };

    mosquitto_user_data_set(mMosq, &connCallbackData);

    cond.wait();

    if (MOSQ_ERR_SUCCESS != connCallbackData.reasonCode) {
        return false;
    }

    mosquitto_user_data_set(mMosq, this); // use self from now, needed for message callback
    mosquitto_message_callback_set(mMosq, onMessageCallback);

    if (MOSQ_ERR_SUCCESS != mosquitto_subscribe(mMosq, nullptr, mClientId.toHex().c_str(), 0)) {
        mosquitto_disconnect(mMosq);
        mConnected = false;
        return false;
    }

    if (MOSQ_ERR_SUCCESS != mosquitto_subscribe(mMosq, nullptr, kEventsTopic, 0)) {
        mosquitto_disconnect(mMosq);
        mConnected = false;
        return false;
    }

    if (!login()) {
        mosquitto_disconnect(mMosq);
        mConnected = false;
        return false;
    }

    return true;
}

void MqttMobilusGtwClientImpl::disconnect()
{
    clearSession();

    if (mConnected) {
        mConfig.clientWatcher->unwatchSocket(this, mosquitto_socket(mMosq));
        mConfig.clientWatcher->unwatchTimer(this);

        mosquitto_disconnect(mMosq);
        mConnected = false;
    }
}

bool MqttMobilusGtwClientImpl::send(const google::protobuf::MessageLite& message)
{
    if (!mConnected) {
        return false;
    }

    Envelope envelope = envelopeFor(message);

    if (MessageType::LoginRequest != envelope.messageType) {
        if (!mPrivateEncryptor) {
            return false;
        }

        envelope.messageBody = mPrivateEncryptor->encrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));
    }

    auto payload = envelope.serialize();

    if (MOSQ_ERR_SUCCESS != mosquitto_publish(mMosq, nullptr, kRequestsTopic, static_cast<int>(payload.size()), payload.data(), 0, false)) {
        return false;
    }

    bumpLastActivity();

    return true;
}

bool MqttMobilusGtwClientImpl::sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response)
{
    if (!send(request)) {
        return false;
    }

    SelectCondition cond(*this, mosquitto_socket(mMosq));
    ExpectedMessage expectedMessage = {
        .cond = cond,
        .expectedMessageType = ProtoUtils::messageTypeFor(response),
        .expectedMessage = response,
    };
    mExpectedMessage = &expectedMessage;

    cond.wait();

    mExpectedMessage = nullptr;

    if (Envelope::ResponseStatus::AuthenticationFailed == expectedMessage.responseStatus) {
        if (!login()) {
            return false;
        }

        return sendRequest(request, response);
    }

    if (Envelope::ResponseStatus::Success != expectedMessage.responseStatus) {
        return false;
    }

    return true;
}

io::SocketEvents MqttMobilusGtwClientImpl::socketEvents()
{
    if (!mConnected) {
        return {};
    }

    io::SocketEvents events;
    events.set(io::SocketEvents::Read);

    if (mosquitto_want_write(mMosq)) {
        events.set(io::SocketEvents::Write);
    }

    return events;
}

void MqttMobilusGtwClientImpl::handleSocketEvents(io::SocketEvents revents)
{
    if (!mConnected) {
        return;
    }

    if (revents.has(io::SocketEvents::Read)) {
        int rc = mosquitto_loop_read(mMosq, 1);

        if (MOSQ_ERR_SUCCESS != rc) {
            handleLostConnection();
            return;
        }

        bumpLastActivity();
        processQueuedMessages();
    }

    if (revents.has(io::SocketEvents::Write)) {
        int rc = mosquitto_loop_write(mMosq, 1);

        if (MOSQ_ERR_SUCCESS != rc) {
            handleLostConnection();
            return;
        }

        bumpLastActivity();
    }
}

void MqttMobilusGtwClientImpl::handleTimerEvent()
{
    if (!mConnected) {
        reconnect();
        return;
    }

    int rc = mosquitto_loop_misc(mMosq);

    if (MOSQ_ERR_SUCCESS != rc) {
        handleLostConnection();
        return;
    }

    bumpLastActivity();
}

void MqttMobilusGtwClientImpl::onConnectCallback(mosquitto* mosq, void* obj, int reasonCode)
{
    auto ctx = reinterpret_cast<ConnectCallbackContext*>(obj);

    if (!ctx) {
        return;
    }

    ctx->reasonCode = reasonCode;
    ctx->cond.notify();
}

void MqttMobilusGtwClientImpl::onMessageCallback(mosquitto* mosq, void* obj, const mosquitto_message* mosqMessage)
{
    auto self = reinterpret_cast<MqttMobilusGtwClientImpl*>(obj);

    if (!self) {
        return;
    }

    self->onMessage(mosqMessage);
}

bool MqttMobilusGtwClientImpl::login()
{
    proto::LoginRequest request;
    proto::LoginResponse response;
    auto hashedPassword = crypto::sha256(mConfig.password);

    request.set_login(mConfig.username);
    request.set_password(hashedPassword.data(), hashedPassword.size());

    mPrivateEncryptor = encryptorFor(std::move(hashedPassword));

    if (!sendRequest(request, response)) {
        return false;
    }

    if (0 != response.login_status()) {
        return false;
    }

    auto publicKey = crypto::bytes(response.public_key().begin(), response.public_key().end());
    auto privateKey = crypto::bytes(response.private_key().begin(), response.private_key().end());

    mPublicEncryptor = encryptorFor(publicKey);
    mPrivateEncryptor = encryptorFor(privateKey);
    mSessionInfo = {
        .userId = response.user_id(),
        .admin = response.admin(),
        .publicKey = std::move(publicKey),
        .privateKey = std::move(privateKey),
        .serialNumber = response.serial_number(),
    };

    return true;
}

void MqttMobilusGtwClientImpl::onMessage(const mosquitto_message* mosqMessage)
{
    if (!strcmp(kEventsTopic, mosqMessage->topic)) {
        return onGeneralMessage(mosqMessage);
    }

    if (!mClientId.toHex().compare(mosqMessage->topic)) {
        if (mExpectedMessage) {
            return onExpectedMessage(*mExpectedMessage, mosqMessage);
        }

        return onGeneralMessage(mosqMessage);
    }
}

void MqttMobilusGtwClientImpl::onGeneralMessage(const mosquitto_message* mosqMessage)
{
    auto envelope = Envelope::deserialize(reinterpret_cast<uint8_t*>(mosqMessage->payload), mosqMessage->payloadlen);

    mConfig.onRawMessage(envelope);

    if (Envelope::ResponseStatus::Success != envelope.responseStatus) {
        mMessageQueue.push({ mosqMessage->topic, std::move(envelope), nullptr });
        return;
    }

    auto& encryptor = MessageType::CallEvents == envelope.messageType ? mPublicEncryptor : mPrivateEncryptor;
    if (!encryptor) {
        return;
    }

    auto message = ProtoUtils::newMessageFor(envelope.messageType);
    if (!message) {
        return;
    }

    auto decryptedBody = encryptor->decrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));
    if (!message->ParseFromArray(decryptedBody.data(), static_cast<int>(decryptedBody.size()))) {
        return;
    }

    mMessageQueue.push({ mosqMessage->topic, std::move(envelope), std::move(message) });
}

void MqttMobilusGtwClientImpl::onExpectedMessage(ExpectedMessage& expectedMessage, const mosquitto_message* mosqMessage)
{
    auto& cond = expectedMessage.cond;
    auto envelope = Envelope::deserialize(reinterpret_cast<uint8_t*>(mosqMessage->payload), mosqMessage->payloadlen);

    mConfig.onRawMessage(envelope);

    expectedMessage.responseStatus = envelope.responseStatus;

    if (Envelope::ResponseStatus::Success != envelope.responseStatus) {
        cond.notify();
        return;
    }

    if (envelope.messageType != expectedMessage.expectedMessageType) {
        cond.notify();
        return;
    }

    if (!mPrivateEncryptor) {
        cond.notify();
        return;
    }

    auto decryptedBody = mPrivateEncryptor->decrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));

    if (!expectedMessage.expectedMessage.ParseFromArray(decryptedBody.data(), static_cast<int>(decryptedBody.size()))) {
        cond.notify();
        return;
    }

    cond.notify();
}

void MqttMobilusGtwClientImpl::handleClientCallEvents(const proto::CallEvents& callEvents)
{
    if (!callEvents.events_size() || EventNumber::Session != callEvents.events(0).event_number()) {
        return;
    }

    auto& event = callEvents.events(0);

    if (!event.value().compare("EXPIRED")) {
        login();
        return;
    }

    char* end = nullptr;
    int remaningTime = static_cast<int>(strtol(event.value().c_str(), &end, 10));

    if (!event.value().empty() && *end == '\0') {
        mConfig.onSessionExpiring(remaningTime);

        if (mConfig.keepAliveMessage) {
            send(*mConfig.keepAliveMessage);
        }
    }
}

void MqttMobilusGtwClientImpl::handleBadMessage(const Envelope& envelope)
{
    if (Envelope::ResponseStatus::AuthenticationFailed == envelope.responseStatus) {
        login();
    }
}

void MqttMobilusGtwClientImpl::handleLostConnection()
{
    if (mReconnecting) {
        return;
    }

    mConfig.clientWatcher->unwatchSocket(this, mosquitto_socket(mMosq));
    mConfig.clientWatcher->watchTimer(this, mReconnectDelay.delay());

    mConnected = false;
    mReconnecting = true;
    clearSession();
}

int MqttMobilusGtwClientImpl::connectMqtt()
{
    if (nullptr != mMosq) {
        return mosquitto_reconnect(mMosq);
    }

    mMosq = mosquitto_new(mClientId.toHex().c_str(), true, nullptr);

    if (nullptr == mMosq) {
        return errno;
    }

    int rc;

    if (mConfig.cafile) {
        rc = mosquitto_tls_set(mMosq, mConfig.cafile->c_str(), nullptr, nullptr, nullptr, nullptr);
        if (MOSQ_ERR_SUCCESS != rc) {
            return rc;
        }

        rc = mosquitto_tls_insecure_set(mMosq, true);
        if (MOSQ_ERR_SUCCESS != rc) {
            return rc;
        }
    }

    mosquitto_connect_callback_set(mMosq, onConnectCallback);

    rc = mosquitto_connect(mMosq, mConfig.host.c_str(), mConfig.port, kKeepAliveIntervalSecs);

    return rc;
}

void MqttMobilusGtwClientImpl::reconnect()
{
    if (!mReconnecting) {
        return;
    }

    if (!connect()) {
        mReconnectDelay.next();
        mConfig.clientWatcher->watchTimer(this, mReconnectDelay.delay());
        return;
    }

    mReconnectDelay.reset();
    mReconnecting = false;
}

void MqttMobilusGtwClientImpl::processQueuedMessages()
{
    for (; !mMessageQueue.empty(); mMessageQueue.pop()) {
        auto& [messageTopic, envelope, message] = mMessageQueue.front();

        if (!message) {
            handleBadMessage(envelope);
            continue;
        }

        // session call events
        if (!messageTopic.compare(mClientId.toHex()) && MessageType::CallEvents == envelope.messageType) {
            handleClientCallEvents(static_cast<const proto::CallEvents&>(*message));
            continue;
        }

        mMessageBus.dispatch(*message);
    }
}

void MqttMobilusGtwClientImpl::clearSession()
{
    mPublicEncryptor = nullptr;
    mPrivateEncryptor = nullptr;
    mSessionInfo.reset();
}

void MqttMobilusGtwClientImpl::bumpLastActivity()
{
    mLastActivity = steady_clock::now();
    mConfig.clientWatcher->watchTimer(this, keepAliveTimerDelay());
}

std::chrono::milliseconds MqttMobilusGtwClientImpl::keepAliveTimerDelay()
{
    std::chrono::seconds keepAliveInterval(kKeepAliveIntervalSecs);
    std::chrono::seconds threshold(1); // safety threshold against infinite loops

    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(keepAliveInterval + mLastActivity - steady_clock::now());

    return diff > threshold ? diff : threshold;
}

Envelope MqttMobilusGtwClientImpl::envelopeFor(const google::protobuf::MessageLite& message)
{
    std::vector<uint8_t> messageBody;

    messageBody.resize(message.ByteSize());
    message.SerializeToArray(messageBody.data(), message.ByteSize());

    return {
        .messageType = ProtoUtils::messageTypeFor(message),
        .timestamp = static_cast<uint32_t>(time(nullptr)),
        .clientId = mClientId,
        .platform = Platform::Web,
        .responseStatus = 0,
        .messageBody = messageBody,
    };
}

std::unique_ptr<crypto::Encryptor> MqttMobilusGtwClientImpl::encryptorFor(crypto::bytes key)
{
    return std::make_unique<crypto::Aes256Encryptor>(std::move(key));
}

MqttMobilusGtwClientImpl::LibInit::LibInit()
{
    mosquitto_lib_init();
}

MqttMobilusGtwClientImpl::LibInit::~LibInit()
{
    mosquitto_lib_cleanup();
    google::protobuf::ShutdownProtobufLibrary();
}

}
