#include "MqttMobilusGtwClientImpl.h"
#include "jungi/mobilus_gtw_client/ErrorCode.h"
#include "jungi/mobilus_gtw_client/EventNumber.h"
#include "jungi/mobilus_gtw_client/Platform.h"
#include "jungi/mobilus_gtw_client/SessionInformation.h"
#include "jungi/mobilus_gtw_client/io/SelectEventLoop.h"
#include "jungi/mobilus_gtw_client/proto/CallEvents.pb.h"
#include "jungi/mobilus_gtw_client/proto/CurrentStateResponse.pb.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListResponse.pb.h"
#include "mocks/MockMqttMobilusActor.h"

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>

using namespace jungi::mobilus_gtw_client;
using jungi::mobilus_gtw_client::tests::mocks::MockMqttMobilusActor;

namespace {

static const auto kDsn = MqttDsn::from("mqtt://localhost:1883?mobilus_username=admin&mobilus_password=admin").value();
static constexpr std::chrono::milliseconds kTimeout(100);

void fakeMqttBroker(std::condition_variable* cv, std::mutex* mutex, bool* ready)
{
    std::unique_lock<std::mutex> lock(*mutex);

    sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(2883);
    saddr.sin_addr.s_addr = INADDR_ANY;

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    bind(serverFd, reinterpret_cast<sockaddr*>(&saddr), sizeof(saddr));
    listen(serverFd, 1);

    *ready = true;
    lock.unlock();
    cv->notify_one();

    int clientFd = accept(serverFd, nullptr, nullptr);
    cv->wait(lock, [&]() -> bool { return ready; });

    close(clientFd);
    close(serverFd);
}

auto callEventsStub()
{
    proto::CallEvents callEvents;

    auto event = callEvents.add_events();
    event->set_id(1);
    event->set_device_id(2);
    event->set_event_number(3);
    event->set_platform(4);

    return callEvents;
}

auto sessionExpiresCallEventsStub(int timeLeft = 60)
{
    proto::CallEvents callEvents;

    auto event = callEvents.add_events();
    event->set_id(1);
    event->set_event_number(EventNumber::Session);
    event->set_value(std::to_string(timeLeft));
    event->set_platform(Platform::Host);

    return callEvents;
}

auto sessionExpiredCallEventsStub()
{
    proto::CallEvents callEvents;

    auto event = callEvents.add_events();
    event->set_id(1);
    event->set_event_number(EventNumber::Session);
    event->set_value("EXPIRED");
    event->set_platform(Platform::Host);

    return callEvents;
}

auto devicesListResponseStub()
{
    proto::DevicesListResponse response;

    auto device = response.add_devices();
    device->set_id(1);
    device->set_name("foo");
    device->set_type(1);

    return response;
}

}

TEST(MqttMobilusGtwClientImplTest, Connects)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    auto r = client.connect();
    auto sessionInfo = client.sessionInfo();

    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(sessionInfo.has_value());
}

TEST(MqttMobilusGtwClientImplTest, ConnectionTimedOut)
{
    bool ready = false;
    std::mutex mutex;
    std::condition_variable cv;

    MqttMobilusGtwClientImpl client(MqttDsn::from("mqtt://localhost:2883?mobilus_username=admin&mobilus_password=admin").value(), std::chrono::milliseconds(1), kTimeout);
    std::thread fakeBroker(fakeMqttBroker, &cv, &mutex, &ready);

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&]() -> bool { return ready; });
    ready = false;

    auto r = client.connect();

    ready = true;
    lock.unlock();
    cv.notify_one();

    if (fakeBroker.joinable()) {
        fakeBroker.join();
    }

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::ConnectionTimeout, r.error().code());
    ASSERT_EQ("MQTT connection timeout: CONNACK missing", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, LoginTimedOut)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, std::chrono::milliseconds(1));

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::LoginTimeout, r.error().code());
    ASSERT_EQ("Login timed out", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, LoginFailed)
{
    MqttMobilusGtwClientImpl client(MqttDsn::from("mqtt://localhost:1883?mobilus_username=admin&mobilus_password=123456").value(), kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::LoginFailed, r.error().code());
    ASSERT_EQ("Mobilus authentication has failed, possibly wrong credentials provided", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, HostCannotBeResolved)
{
    MqttMobilusGtwClientImpl client(MqttDsn::from("mqtt://255.255.255.255:1883?mobilus_username=admin&mobilus_password=admin").value(), kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
    ASSERT_EQ("MQTT connection failed: Network is unreachable", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, InvalidPort)
{
    MqttMobilusGtwClientImpl client(MqttDsn::from("mqtt://localhost:2883?mobilus_username=admin&mobilus_password=admin").value(), kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
    ASSERT_EQ("MQTT connection failed: Connection refused", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, DisconnectsAndConnects)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.disconnect());
    ASSERT_TRUE(client.connect());
}

TEST(MqttMobilusGtwClientImplTest, SendsRequestAndWaitsForResponse)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::DevicesListResponse expectedResponse = devicesListResponseStub();
    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(expectedResponse));

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(expectedResponse.SerializeAsString(), response.SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, SendRequestFailsForUnexpectedResponse)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::CurrentStateResponse>());

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::UnexpectedMessage, r.error().code());
    ASSERT_EQ("Expected to get message of code: 4 but got: 27", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, SendRequestResponseTimeouts)
{
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::ResponseTimeout, r.error().code());
    ASSERT_EQ("Response timed out", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, SubscribesMessage)
{
    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::CallEvents expectedCallEvents = callEventsStub();
    proto::CallEvents actualCallEvents;

    client.messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, [&](const auto& callEvents) {
        actualCallEvents.CopyFrom(callEvents);
        (void)client.disconnect();
    });

    ASSERT_TRUE(client.connect());

    mobilusActor.share(std::make_unique<proto::CallEvents>(expectedCallEvents));
    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_EQ(actualCallEvents.SerializeAsString(), expectedCallEvents.SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, CallsRawMessageCallback)
{
    Envelope actualEnvelope;

    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    client.onRawMessage([&](const Envelope& envelope) { actualEnvelope = envelope; });

    MockMqttMobilusActor mobilusActor("localhost", 1883);

    ASSERT_TRUE(client.connect());

    mobilusActor.share(std::make_unique<proto::CallEvents>(callEventsStub()));
    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_EQ(MessageType::CallEvents, actualEnvelope.messageType);
}

TEST(MqttMobilusGtwClientImplTest, SubscribesAllMessages)
{
    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    std::vector<std::unique_ptr<google::protobuf::MessageLite>> subscribedMessages;
    proto::CallEvents expectedCallEvents = callEventsStub();
    proto::DevicesListResponse expectedDeviceList = devicesListResponseStub();
    proto::CallEvents actualCallEvents;

    client.messageBus().subscribeAll([&](const google::protobuf::MessageLite& message) {
        std::unique_ptr<google::protobuf::MessageLite> subscribedMessage(message.New());

        subscribedMessage->CheckTypeAndMergeFrom(message);
        subscribedMessages.push_back(std::move(subscribedMessage));
    });
    client.messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, [&](const auto& message) {
        actualCallEvents.CopyFrom(message);
    });

    ASSERT_TRUE(client.connect());

    mobilusActor.share(std::make_unique<proto::CallEvents>(expectedCallEvents));
    mobilusActor.reply(std::make_unique<proto::DevicesListResponse>(expectedDeviceList));

    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_EQ(2, subscribedMessages.size());
    ASSERT_EQ(expectedCallEvents.SerializeAsString(), actualCallEvents.SerializeAsString());
    ASSERT_EQ(expectedCallEvents.SerializeAsString(), subscribedMessages[0]->SerializeAsString());
    ASSERT_EQ(expectedDeviceList.SerializeAsString(), subscribedMessages[1]->SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, ExpectedResponseIsNotSubscribed)
{
    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(devicesListResponseStub()));

    bool subscribed = false;
    client.messageBus().subscribe<proto::DevicesListResponse>(MessageType::DevicesListResponse, [&](const auto&) {
        subscribed = true;
    });

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    ASSERT_TRUE(client.sendRequest(proto::DevicesListRequest(), response));

    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_FALSE(subscribed);
}

TEST(MqttMobilusGtwClientImplTest, SendsKeepAliveMessageOnExpiringSession)
{
    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    client.useKeepAliveMessage(std::make_unique<proto::DevicesListRequest>());

    MockMqttMobilusActor mobilusActor("localhost", 1883);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(devicesListResponseStub()));

    bool received = false;
    client.messageBus().subscribe<proto::DevicesListResponse>(MessageType::DevicesListResponse, [&](const auto&) {
        received = true;
        (void)client.disconnect();
    });

    ASSERT_TRUE(client.connect());

    mobilusActor.reply(std::make_unique<proto::CallEvents>(sessionExpiresCallEventsStub()));
    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_TRUE(received);
}

TEST(MqttMobilusGtwClientImplTest, CallsSessionExpiringCallback)
{
    int expectedTimeLeft = 15;
    int actualTimeLeft = 0;

    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    client.onSessionExpiring([&](int timeLeft) { actualTimeLeft = timeLeft; });

    MockMqttMobilusActor mobilusActor("localhost", 1883);

    ASSERT_TRUE(client.connect());

    mobilusActor.reply(std::make_unique<proto::CallEvents>(sessionExpiresCallEventsStub(15)));
    loop.runFor(std::chrono::milliseconds(100));

    ASSERT_EQ(expectedTimeLeft, actualTimeLeft);
}

TEST(MqttMobilusGtwClientImplTest, ReconnectsOnExpiredSession)
{
    io::SelectEventLoop loop;
    MqttMobilusGtwClientImpl client(kDsn, kTimeout, kTimeout, loop);
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    ASSERT_TRUE(client.connect());
    SessionInformation sessionInfo = *client.sessionInfo();

    mobilusActor.reply(std::make_unique<proto::CallEvents>(sessionExpiredCallEventsStub()));
    loop.runFor(std::chrono::milliseconds(150));

    ASSERT_TRUE(client.sessionInfo().has_value());
    ASSERT_NE(sessionInfo.publicKey, client.sessionInfo()->publicKey);
    ASSERT_NE(sessionInfo.privateKey, client.sessionInfo()->privateKey);
}
