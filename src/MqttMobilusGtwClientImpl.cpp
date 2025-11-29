#include "MqttMobilusGtwClientImpl.h"
#include "Qos.h"
#include "crypto/EvpEncryptor.h"
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

using jungi::mobilus_gtw_client::crypto::EvpEncryptor;
using std::chrono::steady_clock;

static constexpr char kEventsTopic[] = "clients";
static constexpr char kRequestsTopic[] = "module";
static constexpr int kKeepAliveIntervalSecs = 60;
static const auto kEncryptor = EvpEncryptor::Aes256Cfb128();

namespace jungi::mobilus_gtw_client {

MqttMobilusGtwClientImpl::MqttMobilusGtwClientImpl(MqttDsn dsn, MobilusCredentials mobilusCreds, std::chrono::milliseconds conenctTimeout, std::chrono::milliseconds responseTimeout, io::EventLoop& loop, logging::Logger& logger)
    : mClientId(ClientId::unique())
    , mDsn(std::move(dsn))
    , mMobilusCreds(std::move(mobilusCreds))
    , mConnectTimeout(conenctTimeout)
    , mResponseTimeout(responseTimeout)
    , mLoop(loop)
    , mLogger(logger)
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

void MqttMobilusGtwClientImpl::useKeepAliveMessage(std::unique_ptr<google::protobuf::MessageLite> message)
{
    mKeepAliveMessage = std::move(message);
}

void MqttMobilusGtwClientImpl::onSessionExpiring(SessionExpiringCallback callback)
{
    mSessionExpiringCallback = std::move(callback);
}

void MqttMobilusGtwClientImpl::onRawMessage(RawMessageCallback callback)
{
    mRawMessageCallback = std::move(callback);
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::connect()
{
    int rc;

    if (mConnected) {
        return {};
    }

    if (MOSQ_ERR_SUCCESS != (rc = connectMqtt())) {
        return logAndReturn(Error::Transport("MQTT connection failed: " + std::string(mosquitto_strerror(rc))));
    }

    mConnected = true;
    mLoop.watchSocket(mosquitto_socket(mMosq), this);

    SelectCondition cond(*this, mosquitto_socket(mMosq), mConnectTimeout);
    ConnectCallbackContext connCallbackData = { cond, -1 };

    mosquitto_user_data_set(mMosq, &connCallbackData);

    cond.wait();

    if (!cond.condition()) {
        return logAndReturn(Error::ConnectionTimeout("MQTT connection timeout: CONNACK missing"));
    } else if (MOSQ_ERR_SUCCESS != connCallbackData.returnCode) {
        return logAndReturn(Error::ConnectionRefused("MQTT connection refused: " + explainConnackCode(connCallbackData.returnCode)));
    }

    mLogger.info("MQTT connected to broker");

    mosquitto_user_data_set(mMosq, this); // use self from now, needed for message callback
    mosquitto_message_callback_set(mMosq, onMessageCallback);

    if (MOSQ_ERR_SUCCESS != (rc = mosquitto_subscribe(mMosq, nullptr, mClientId.toHex().c_str(), Qos::AtLeastOnce))) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return logAndReturn(Error::Transport("MQTT subscription to client queue failed due to: " + std::string(mosquitto_strerror(rc))));
    }

    if (auto e = login(); !e) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return e;
    }

    if (MOSQ_ERR_SUCCESS != (rc = mosquitto_subscribe(mMosq, nullptr, kEventsTopic, Qos::AtMostOnce))) {
        mosquitto_disconnect(mMosq);
        mConnected = false;

        return logAndReturn(Error::Transport("MQTT subscription to events queue failed due to: " + std::string(mosquitto_strerror(rc))));
    }

    scheduleMisc();
    mLogger.info("Connected to mobilus");

    return {};
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::disconnect()
{
    mSessionInfo.reset();

    if (mConnected) {
        mLoop.unwatchSocket(mosquitto_socket(mMosq));
        mLoop.stopTimer(mReconnectTimerId);
        mLoop.stopTimer(mMiscTimerId);

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
    return send(message, Qos::AtMostOnce);
}

MqttMobilusGtwClient::Result<> MqttMobilusGtwClientImpl::sendRequest(const google::protobuf::MessageLite& request, google::protobuf::MessageLite& response)
{
    if (auto e = send(request, Qos::AtLeastOnce); !e) {
        return e;
    }

    SelectCondition cond(*this, mosquitto_socket(mMosq), mResponseTimeout);
    ExpectedMessage expectedMessage = { cond, ProtoUtils::messageTypeFor(response), response };

    mExpectedMessage = &expectedMessage;
    cond.wait();
    mExpectedMessage = nullptr;

    if (!cond.condition()) {
        return logAndReturn(Error::ResponseTimeout("Response timed out"));
    }

    if (expectedMessage.error) {
        if (ErrorCode::InvalidSession == expectedMessage.error->code()) {
            handleInvalidSession();
        }

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

        dispatchQueuedMessages();
    }

    if (revents.has(io::SocketEvents::Write)) {
        int rc = mosquitto_loop_write(mMosq, 1);

        if (MOSQ_ERR_SUCCESS != rc) {
            handleLostConnection(rc);
            return;
        }
    }
}

void MqttMobilusGtwClientImpl::handleMisc()
{
    if (!mConnected) {
        return;
    }

    int rc = mosquitto_loop_misc(mMosq);

    if (MOSQ_ERR_SUCCESS != rc) {
        handleLostConnection(rc);
        return;
    }

    scheduleMisc();
}

void MqttMobilusGtwClientImpl::onConnectCallback(mosquitto*, void* obj, int returnCode)
{
    auto ctx = reinterpret_cast<ConnectCallbackContext*>(obj);

    if (!ctx) {
        return;
    }

    ctx->returnCode = returnCode;
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
        if (!mSessionInfo) {
            return logAndReturn(Error::NoSession("There is no open session"));
        }

        envelope.messageBody = kEncryptor.encrypt(envelope.messageBody, mSessionInfo->privateKey, crypto::timestamp2iv(envelope.timestamp));
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
    auto hashedPassword = crypto::sha256(mMobilusCreds.password);

    request.set_login(mMobilusCreds.username);
    request.set_password(hashedPassword.data(), hashedPassword.size());

    // mPrivateEncryptor =

    if (auto e = sendRequest(request, response); !e) {
        if (ErrorCode::ResponseTimeout == e.error().code()) {
            return logAndReturn(Error::LoginTimeout("Login timed out"));
        }

        return logAndReturn(Error::LoginFailed("Login request has failed: " + e.error().message()));
    }

    if (0 != response.login_status()) {
        return logAndReturn(Error::LoginFailed("Mobilus authentication has failed, possibly wrong credentials provided"));
    }

    mSessionInfo = {
        response.user_id(),
        response.admin(),
        crypto::bytes(response.public_key().begin(), response.public_key().end()),
        crypto::bytes(response.private_key().begin(), response.private_key().end()),
        response.serial_number(),
    };

    return {};
}

void MqttMobilusGtwClientImpl::onMessage(const mosquitto_message* mosqMessage)
{
    if (!strcmp(kEventsTopic, mosqMessage->topic)) {
        return onGeneralMessage(mosqMessage);
    }

    if (mClientId.toHex() == mosqMessage->topic) {
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
        mLogger.error("Received invalid message of size: " + std::to_string(mosqMessage->payloadlen));
        return;
    }

    mRawMessageCallback(*envelope);

    if (Envelope::ResponseStatus::InvalidSession == envelope->responseStatus) {
        handleInvalidSession();

        mLogger.error("Session is invalid or expired, got status: " + std::to_string(envelope->responseStatus));
        return;
    }

    if (Envelope::ResponseStatus::Success != envelope->responseStatus) {
        mLogger.error("Bad response from mobilus: " + std::to_string(envelope->responseStatus));
        return;
    }

    if (!mSessionInfo) {
        mLogger.error("There is no open session");
        return;
    }

    auto& key = MessageType::CallEvents == envelope->messageType ? mSessionInfo->publicKey : mSessionInfo->privateKey;
    auto message = ProtoUtils::newMessageFor(envelope->messageType);

    if (!message) {
        mLogger.error("Missing proto for message type: " + std::to_string(envelope->messageType));
        return;
    }

    auto decryptedBody = kEncryptor.decrypt(envelope->messageBody, key, crypto::timestamp2iv(envelope->timestamp));

    if (!message->ParseFromArray(decryptedBody.data(), static_cast<int>(decryptedBody.size()))) {
        mLogger.error("Failed to parse proto of message type: " + std::to_string(envelope->messageType));
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

    mRawMessageCallback(*envelope);

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

    if (!mSessionInfo) {
        expectedMessage.error = Error::NoSession("There is no open session");
        cond.notify();

        return;
    }

    auto decryptedBody = kEncryptor.decrypt(envelope->messageBody, mSessionInfo->privateKey, crypto::timestamp2iv(envelope->timestamp));

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

    if ("EXPIRED" == event.value()) {
        handleInvalidSession();
        return;
    }

    char* end = nullptr;
    int remaningTime = static_cast<int>(strtol(event.value().c_str(), &end, 10));

    if (!event.value().empty() && *end == '\0') {
        mLogger.info("Mobilus session expiring in " + std::to_string(remaningTime) + "s");
        mSessionExpiringCallback(remaningTime);

        if (mKeepAliveMessage) {
            (void)send(*mKeepAliveMessage, Qos::AtLeastOnce);
        }
    }
}

void MqttMobilusGtwClientImpl::handleInvalidSession()
{
    if (mReconnecting) {
        return;
    }

    mLogger.error("Mobilus session expired or invalid");

    (void)disconnect();

    mReconnectTimerId = mLoop.startTimer(mReconnectDelay.delay(), reconnectTimerCallback, this);
    mReconnecting = true;
}

void MqttMobilusGtwClientImpl::handleLostConnection(int rc)
{
    if (mReconnecting) {
        return;
    }

    mLogger.error("MQTT connection lost: " + std::string(mosquitto_strerror(rc)));

    mLoop.unwatchSocket(mosquitto_socket(mMosq));
    mReconnectTimerId = mLoop.startTimer(mReconnectDelay.delay(), reconnectTimerCallback, this);

    mConnected = false;
    mReconnecting = true;
    mSessionInfo.reset();
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

    if (mDsn.secure) {
        if (MOSQ_ERR_SUCCESS != (rc = mosquitto_tls_set(mMosq, mDsn.cacert ? mDsn.cacert->c_str() : nullptr, nullptr, nullptr, nullptr, nullptr))) {
            return rc;
        }
        if (MOSQ_ERR_SUCCESS != (rc = mosquitto_tls_insecure_set(mMosq, mDsn.verify.value_or(true) ? false : true))) {
            return rc;
        }
    }

    rc = mosquitto_username_pw_set(mMosq, mDsn.username ? mDsn.username->c_str() : nullptr, mDsn.password ? mDsn.password->c_str() : nullptr);
    if (MOSQ_ERR_SUCCESS != rc) {
        return rc;
    }

    mosquitto_connect_callback_set(mMosq, onConnectCallback);

    return mosquitto_connect(mMosq, mDsn.host.c_str(), static_cast<int>(*mDsn.port), kKeepAliveIntervalSecs);
}

void MqttMobilusGtwClientImpl::reconnect()
{
    if (!mReconnecting) {
        return;
    }

    mLogger.info("Reconnecting to MQTT broker and mobilus");

    if (!connect()) {
        mReconnectDelay.next();
        mReconnectTimerId = mLoop.startTimer(mReconnectDelay.delay(), reconnectTimerCallback, this);

        return;
    }

    mReconnectDelay.reset();
    mReconnecting = false;
}

void MqttMobilusGtwClientImpl::dispatchQueuedMessages()
{
    while (!mMessageQueue.empty()) {
        auto queuedMessage = std::move(mMessageQueue.front());

        mMessageQueue.pop();

        // session call events
        if (queuedMessage.topic == mClientId.toHex() && MessageType::CallEvents == queuedMessage.messageType) {
            handleClientCallEvents(static_cast<const proto::CallEvents&>(*queuedMessage.message));
            continue;
        }

        mMessageBus.dispatch(*queuedMessage.message);
    }
}

void MqttMobilusGtwClientImpl::scheduleMisc()
{
    mLoop.stopTimer(mMiscTimerId);
    mMiscTimerId = mLoop.startTimer(std::chrono::seconds(1), miscTimerCallback, this); // due to mosquitto_loop_misc() PINGREQ
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

std::string MqttMobilusGtwClientImpl::explainConnackCode(int code)
{
    std::string str = mosquitto_connack_string(code);
    auto rpos = str.rfind(": ");

    return str.substr(rpos != std::string::npos ? rpos + 2 : 0);
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
