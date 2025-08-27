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
    (void)disconnect();

    if (nullptr != mMosq) {
        mosquitto_destroy(mMosq);
        mMosq = nullptr;
    }
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::connect()
{
    int rc;

    if (mConnected) {
        return {};
    }

    rc = connectMqtt();
    if (MOSQ_ERR_SUCCESS != rc) {
        return logAndReturn(Error::Transport("MQTT connection failed: " + std::string(mosquitto_strerror(rc))));
    }

    mConnected = true;
    mConfig.clientWatcher->watchSocket(this, mosquitto_socket(mMosq));

    SelectCondition cond(*this, mosquitto_socket(mMosq), mConfig.connectTimeout);
    ConnectCallbackContext connCallbackData = { cond, -1 };

    mosquitto_user_data_set(mMosq, &connCallbackData);

    cond.wait();

    if (!cond.condition()) {
        return logAndReturn(Error::ConnectionTimeout("MQTT connection timeout: CONNACK missing"));
    } else if (MOSQ_ERR_SUCCESS != connCallbackData.reasonCode) {
        return logAndReturn(Error::Transport("MQTT connection failed: " + std::string(mosquitto_strerror(rc))));
    }

    mConfig.logger->info("MQTT connected to broker");

    mosquitto_user_data_set(mMosq, this); // use self from now, needed for message callback
    mosquitto_message_callback_set(mMosq, onMessageCallback);

    rc = mosquitto_subscribe(mMosq, nullptr, mClientId.toHex().c_str(), 0);
    if (MOSQ_ERR_SUCCESS != rc) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return logAndReturn(Error::Transport("MQTT subscription to client queue failed due to: " + std::string(mosquitto_strerror(rc))));
    }

    rc = mosquitto_subscribe(mMosq, nullptr, kEventsTopic, 0);
    if (MOSQ_ERR_SUCCESS != rc) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return logAndReturn(Error::Transport("MQTT subscription to events queue failed due to: " + std::string(mosquitto_strerror(rc))));
    }

    if (auto e = login(); !e) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return e;
    }

    scheduleTimer();
    mConfig.logger->info("Connected to mobilus");

    return {};
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::disconnect()
{
    clearSession();

    if (mConnected) {
        mConfig.clientWatcher->unwatchSocket(this, mosquitto_socket(mMosq));
        mConfig.clientWatcher->unwatchTimer(this);

        int rc = mosquitto_disconnect(mMosq);
        mConnected = false;

        if (MOSQ_ERR_SUCCESS != rc) {
            return logAndReturn(Error::Transport("MQTT disconnect failed: " + std::string(mosquitto_strerror(rc))));
        }
    }

    return {};
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::send(const google::protobuf::MessageLite& message)
{
    return send(message, 0);
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response)
{
    if (auto e = send(request, 1); !e) {
        return e;
    }

    SelectCondition cond(*this, mosquitto_socket(mMosq), mConfig.responseTimeout);
    ExpectedMessage expectedMessage = { cond, ProtoUtils::messageTypeFor(response), response };

    mExpectedMessage = &expectedMessage;
    cond.wait();
    mExpectedMessage = nullptr;

    if (!cond.condition()) {
        return logAndReturn(Error::ResponseTimeout("Response timed out"));
    }

    if (expectedMessage.error) {
        return logAndReturn(std::move(*expectedMessage.error));
    }

    return {};
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
            handleLostConnection(rc);
            return;
        }

        processQueuedMessages();
    }

    if (revents.has(io::SocketEvents::Write)) {
        int rc = mosquitto_loop_write(mMosq, 1);

        if (MOSQ_ERR_SUCCESS != rc) {
            handleLostConnection(rc);
            return;
        }
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
        handleLostConnection(rc);
        return;
    }

    scheduleTimer();
}

void MqttMobilusGtwClientImpl::onConnectCallback(mosquitto*, void* obj, int reasonCode)
{
    auto ctx = reinterpret_cast<ConnectCallbackContext*>(obj);

    if (!ctx) {
        return;
    }

    ctx->reasonCode = reasonCode;
    ctx->cond.notify();
}

void MqttMobilusGtwClientImpl::onMessageCallback(mosquitto*, void* obj, const mosquitto_message* mosqMessage)
{
    auto self = reinterpret_cast<MqttMobilusGtwClientImpl*>(obj);

    if (!self) {
        return;
    }

    self->onMessage(mosqMessage);
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::send(const google::protobuf::MessageLite& message, int qos)
{
    if (!mConnected) {
        return logAndReturn(Error::NoConnection("Not connected"));
    }

    Envelope envelope = envelopeFor(message);

    if (MessageType::LoginRequest != envelope.messageType) {
        if (!mPrivateEncryptor) {
            return logAndReturn(Error::Unknown("Private key is missing which should not happen in this client state"));
        }

        envelope.messageBody = mPrivateEncryptor->encrypt(envelope.messageBody, crypto::timestamp2iv(envelope.timestamp));
    }

    auto payload = envelope.serialize();
    int rc = mosquitto_publish(mMosq, nullptr, kRequestsTopic, static_cast<int>(payload.size()), payload.data(), qos, false);

    if (MOSQ_ERR_SUCCESS != rc) {
        return logAndReturn(Error::Transport("MQTT publish failed: " + std::string(mosquitto_strerror(rc))));
    }

    return {};
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::login()
{
    proto::LoginRequest request;
    proto::LoginResponse response;
    auto hashedPassword = crypto::sha256(mConfig.password);

    request.set_login(mConfig.username);
    request.set_password(hashedPassword.data(), hashedPassword.size());

    mPrivateEncryptor = encryptorFor(std::move(hashedPassword));

    if (auto e = sendRequest(request, response); !e) {
        if (ErrorCode::ResponseTimeout == e.error().code()) {
            return logAndReturn(Error::LoginTimeout("Login timed out"));
        }

        return logAndReturn(Error::LoginFailed("Login request has failed: " + e.error().message()));
    }

    if (0 != response.login_status()) {
        return logAndReturn(Error::LoginFailed("Mobilus authentication has failed, possibly wrong credentials provided"));
    }

    auto publicKey = crypto::bytes(response.public_key().begin(), response.public_key().end());
    auto privateKey = crypto::bytes(response.private_key().begin(), response.private_key().end());

    mPublicEncryptor = encryptorFor(publicKey);
    mPrivateEncryptor = encryptorFor(privateKey);
    mSessionInfo = {
        response.user_id(),
        response.admin(),
        std::move(publicKey),
        std::move(privateKey),
        response.serial_number(),
    };

    return {};
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
    auto envelope = Envelope::deserialize(reinterpret_cast<uint8_t*>(mosqMessage->payload), static_cast<uint32_t>(mosqMessage->payloadlen));
    if (!envelope) {
        mMessageQueue.push({ mosqMessage->topic, MessageType::Unknown, nullptr, Error::BadMessage("Received invalid message of size: " + std::to_string(mosqMessage->payloadlen)) });
        return;
    }

    mConfig.onRawMessage(*envelope);

    if (Envelope::ResponseStatus::InvalidSession == envelope->responseStatus) {
        mMessageQueue.push({ mosqMessage->topic, envelope->messageType, nullptr, Error::InvalidSession("Session is invalid or expired, got status: " + std::to_string(envelope->responseStatus)) });
        return;
    }

    if (Envelope::ResponseStatus::Success != envelope->responseStatus) {
        mMessageQueue.push({ mosqMessage->topic, envelope->messageType, nullptr, Error::BadResponse("Bad response from mobilus: " + std::to_string(envelope->responseStatus)) });
        return;
    }

    auto& encryptor = MessageType::CallEvents == envelope->messageType ? mPublicEncryptor : mPrivateEncryptor;
    if (!encryptor) {
        mMessageQueue.push({ mosqMessage->topic, envelope->messageType, nullptr, Error::Unknown("Missing encryptor for message type: " + std::to_string(envelope->messageType)) });
        return;
    }

    auto message = ProtoUtils::newMessageFor(envelope->messageType);
    if (!message) {
        mMessageQueue.push({ mosqMessage->topic, envelope->messageType, nullptr, Error::Unknown("Missing proto for message type: " + std::to_string(envelope->messageType)) });
        return;
    }

    auto decryptedBody = encryptor->decrypt(envelope->messageBody, crypto::timestamp2iv(envelope->timestamp));

    if (!message->ParseFromArray(decryptedBody.data(), static_cast<int>(decryptedBody.size()))) {
        mMessageQueue.push({ mosqMessage->topic, envelope->messageType, nullptr, Error::BadMessage("Failed to parse proto of message type: " + std::to_string(envelope->messageType)) });
        return;
    }

    mMessageQueue.push({ mosqMessage->topic, envelope->messageType, std::move(message) });
}

void MqttMobilusGtwClientImpl::onExpectedMessage(ExpectedMessage& expectedMessage, const mosquitto_message* mosqMessage)
{
    auto& cond = expectedMessage.cond;
    auto envelope = Envelope::deserialize(reinterpret_cast<uint8_t*>(mosqMessage->payload), static_cast<uint32_t>(mosqMessage->payloadlen));

    if (!envelope) {
        expectedMessage.error = Error::BadMessage("Received invalid message of size: " + std::to_string(mosqMessage->payloadlen));
        cond.notify();

        return;
    }

    mConfig.onRawMessage(*envelope);

    expectedMessage.responseStatus = envelope->responseStatus;

    if (Envelope::ResponseStatus::InvalidSession == envelope->responseStatus) {
        expectedMessage.error = Error::InvalidSession("Session is invalid or expired, got status: " + std::to_string(envelope->responseStatus));
        cond.notify();

        return;
    }

    if (Envelope::ResponseStatus::Success != envelope->responseStatus) {
        expectedMessage.error = Error::BadResponse("Bad response from mobilus: " + std::to_string(envelope->responseStatus));
        cond.notify();

        return;
    }

    if (envelope->messageType != expectedMessage.expectedMessageType) {
        expectedMessage.error = Error::UnexpectedMessage("Expected to get message of code: " + std::to_string(expectedMessage.expectedMessageType) + " but got: " + std::to_string(envelope->messageType));
        cond.notify();

        return;
    }

    if (!mPrivateEncryptor) {
        expectedMessage.error = Error::Unknown("Missing private encryptor for message type: " + std::to_string(envelope->messageType));
        cond.notify();

        return;
    }

    auto decryptedBody = mPrivateEncryptor->decrypt(envelope->messageBody, crypto::timestamp2iv(envelope->timestamp));

    if (!expectedMessage.expectedMessage.ParseFromArray(decryptedBody.data(), static_cast<int>(decryptedBody.size()))) {
        expectedMessage.error = Error::BadMessage("Failed to parse proto of message type: " + std::to_string(envelope->messageType));
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
        (void)login();
        return;
    }

    char* end = nullptr;
    int remaningTime = static_cast<int>(strtol(event.value().c_str(), &end, 10));

    if (!event.value().empty() && *end == '\0') {
        mConfig.onSessionExpiring(remaningTime);

        if (mConfig.keepAliveMessage) {
            (void)send(*mConfig.keepAliveMessage);
        }
    }
}

void MqttMobilusGtwClientImpl::processError(const Error& error)
{
    mConfig.logger->error(error.message());

    if (ErrorCode::InvalidSession == error.code()) {
        (void)login();
    }
}

void MqttMobilusGtwClientImpl::handleLostConnection(int rc)
{
    if (mReconnecting) {
        return;
    }

    mConfig.logger->error("MQTT connection lost: " + std::string(mosquitto_strerror(rc)));

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
        return MOSQ_ERR_ERRNO;
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

    rc = mosquitto_connect(mMosq, mConfig.host.c_str(), static_cast<int>(mConfig.port), kKeepAliveIntervalSecs);

    return rc;
}

void MqttMobilusGtwClientImpl::reconnect()
{
    if (!mReconnecting) {
        return;
    }

    mConfig.logger->info("Reconnecting to MQTT broker and mobilus");

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
    while (!mMessageQueue.empty()) {
        auto queuedMessage = std::move(mMessageQueue.front());

        mMessageQueue.pop();

        if (queuedMessage.error) {
            processError(*queuedMessage.error);
            continue;
        }

        // session call events
        if (!queuedMessage.topic.compare(mClientId.toHex()) && MessageType::CallEvents == queuedMessage.messageType) {
            handleClientCallEvents(static_cast<const proto::CallEvents&>(*queuedMessage.message));
            continue;
        }

        mMessageBus.dispatch(*queuedMessage.message);
    }
}

void MqttMobilusGtwClientImpl::clearSession()
{
    mPublicEncryptor = nullptr;
    mPrivateEncryptor = nullptr;
    mSessionInfo.reset();
}

void MqttMobilusGtwClientImpl::scheduleTimer()
{
    mConfig.clientWatcher->watchTimer(this, std::chrono::seconds(1)); // due to mosquitto_loop_misc() PINGREQ
}

Envelope MqttMobilusGtwClientImpl::envelopeFor(const google::protobuf::MessageLite& message)
{
    std::vector<uint8_t> messageBody;

    messageBody.resize(static_cast<size_t>(message.ByteSize()));
    message.SerializeToArray(messageBody.data(), message.ByteSize());

    return {
        ProtoUtils::messageTypeFor(message),
        static_cast<uint32_t>(time(nullptr)),
        mClientId.value(),
        Platform::Web,
        0,
        messageBody,
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
