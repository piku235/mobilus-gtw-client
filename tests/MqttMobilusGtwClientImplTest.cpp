#include "MqttMobilusGtwClientImpl.h"
#include "io/TestSocketWatcher.h"
#include "jungi/mobilus_gtw_client/ErrorCode.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClientConfig.h"
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
using jungi::mobilus_gtw_client::tests::io::TestSocketWatcher;
using jungi::mobilus_gtw_client::tests::mocks::MockMqttMobilusActor;

namespace {

auto clientConfig()
{
    MqttMobilusGtwClientConfig config("localhost", 1883, "admin", "admin");
    config.responseTimeout = std::chrono::milliseconds(100);

    return config;
}

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
    MqttMobilusGtwClientImpl client(clientConfig());
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_TRUE(r.has_value());
}

TEST(MqttMobilusGtwClientImplTest, ConnectFailsOnTimeout)
{
    bool ready = false;
    std::mutex mutex;
    std::condition_variable cv;

    MqttMobilusGtwClientConfig config = { "localhost", 2883, "admin", "admin" };
    config.connectTimeout = std::chrono::milliseconds(1);

    MqttMobilusGtwClientImpl client(std::move(config));
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

TEST(MqttMobilusGtwClientImplTest, LoginFailsOnTimeout)
{
    auto config = clientConfig();
    config.responseTimeout = std::chrono::milliseconds(1);

    MqttMobilusGtwClientImpl client(std::move(config));

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::LoginFailed, r.error().code());
    ASSERT_EQ("Login request has failed: Response timed out", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, LoginFailed)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "123456" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::LoginFailed, r.error().code());
    ASSERT_EQ("Mobilus authentication has failed, possibly wrong credentials provided", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, HostCannotBeResolved)
{
    MqttMobilusGtwClientImpl client({ "255.255.255.255", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
    ASSERT_EQ("MQTT connection failed: Network is unreachable", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, InvalidPort)
{
    MqttMobilusGtwClientImpl client({ "localhost", 2883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
    ASSERT_EQ("MQTT connection failed: Connection refused", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, DisconnectsAndConnects)
{
    MqttMobilusGtwClientImpl client(clientConfig());
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.disconnect());
    ASSERT_TRUE(client.connect());
}

TEST(MqttMobilusGtwClientImplTest, SendsRequestAndWaitsForResponse)
{
    MqttMobilusGtwClientImpl client(clientConfig());
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::DevicesListResponse expectedResponse = devicesListResponseStub();
    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(expectedResponse));

    mobilusActor.run();

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_TRUE(r.has_value());
    ASSERT_EQ(expectedResponse.SerializeAsString(), response.SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, SendRequestFailsForUnexpectedResponse)
{
    MqttMobilusGtwClientImpl client(clientConfig());
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::CurrentStateResponse>());
    mobilusActor.run();

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::UnexpectedMessage, r.error().code());
    ASSERT_EQ("Expected to get message of code: 4 but got: 27", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, SendRequestResponseTimeouts)
{
    MqttMobilusGtwClientImpl client(clientConfig());
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::ResponseTimeout, r.error().code());
    ASSERT_EQ("Response timed out", r.error().message());
}

TEST(MqttMobilusGtwClientImplTest, SubscribesMessage)
{
    TestSocketWatcher clientWatcher;
    MqttMobilusGtwClientConfig config = clientConfig();
    config.clientWatcher = &clientWatcher;

    MqttMobilusGtwClientImpl client(std::move(config));
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::CallEvents expectedCallEvents = callEventsStub();
    proto::CallEvents actualCallEvents;

    mobilusActor.mockResponseFor(MessageType::CallEvents, std::make_unique<proto::CallEvents>(expectedCallEvents));
    mobilusActor.run();

    client.messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, [&](const auto& callEvents) {
        actualCallEvents.CopyFrom(callEvents);
        (void)client.disconnect();
    });

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.send(proto::CallEvents()));

    clientWatcher.loopFor(std::chrono::milliseconds(100));

    ASSERT_EQ(actualCallEvents.SerializeAsString(), expectedCallEvents.SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, SubscribesAllMessages)
{
    TestSocketWatcher clientWatcher;
    MqttMobilusGtwClientConfig config = clientConfig();
    config.clientWatcher = &clientWatcher;

    MqttMobilusGtwClientImpl client(std::move(config));
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    std::vector<std::unique_ptr<google::protobuf::MessageLite>> subscribedMessages;
    proto::CallEvents expectedCallEvents = callEventsStub();
    proto::DevicesListResponse expectedDeviceList = devicesListResponseStub();
    proto::CallEvents actualCallEvents;

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(expectedDeviceList));
    mobilusActor.mockResponseFor(MessageType::CallEvents, std::make_unique<proto::CallEvents>(expectedCallEvents));
    mobilusActor.run();

    client.messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, [&actualCallEvents](const auto& message) {
        actualCallEvents.CopyFrom(message);
    });
    client.messageBus().subscribeAll([&subscribedMessages](const google::protobuf::MessageLite& message) {
        std::unique_ptr<google::protobuf::MessageLite> subscribedMessage(message.New());

        subscribedMessage->CheckTypeAndMergeFrom(message);
        subscribedMessages.push_back(std::move(subscribedMessage));
    });

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.send(proto::CallEvents()));
    ASSERT_TRUE(client.send(proto::DevicesListRequest()));

    clientWatcher.loopFor(std::chrono::milliseconds(100));

    ASSERT_EQ(2, subscribedMessages.size());
    ASSERT_EQ(expectedCallEvents.SerializeAsString(), actualCallEvents.SerializeAsString());
    ASSERT_EQ(expectedCallEvents.SerializeAsString(), subscribedMessages[0]->SerializeAsString());
    ASSERT_EQ(expectedDeviceList.SerializeAsString(), subscribedMessages[1]->SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, ExpectedResponseIsNotSubscribed)
{
    TestSocketWatcher clientWatcher;
    MqttMobilusGtwClientConfig config = clientConfig();
    config.clientWatcher = &clientWatcher;

    MqttMobilusGtwClientImpl client(std::move(config));
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    bool subscribed = false;

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(devicesListResponseStub()));
    mobilusActor.run();

    client.messageBus().subscribe<proto::DevicesListResponse>(MessageType::DevicesListResponse, [&subscribed](const auto&) {
        subscribed = true;
    });

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    ASSERT_TRUE(client.sendRequest(proto::DevicesListRequest(), response));

    clientWatcher.loopFor(std::chrono::milliseconds(100));

    ASSERT_FALSE(subscribed);
}
