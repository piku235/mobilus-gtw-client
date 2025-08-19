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
#include <memory>
#include <vector>

using namespace jungi::mobilus_gtw_client;
using jungi::mobilus_gtw_client::tests::io::TestSocketWatcher;
using jungi::mobilus_gtw_client::tests::mocks::MockMqttMobilusActor;

TEST(MqttMobilusGtwClientImplTest, Connects)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_TRUE(r.has_value());
}

TEST(MqttMobilusGtwClientImplTest, AuthenticationFailsOnTimeout)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::AuthenticationFailed, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, AuthenticationFailed)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "123456" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::AuthenticationFailed, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, HostCannotBeResolved)
{
    MqttMobilusGtwClientImpl client({ "255.255.255.255", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, InvalidPort)
{
    MqttMobilusGtwClientImpl client({ "localhost", 2883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    auto r = client.connect();

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Transport, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, DisconnectsAndConnects)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.disconnect());
    ASSERT_TRUE(client.connect());
}

TEST(MqttMobilusGtwClientImplTest, SendsRequestAndWaitsForResponse)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::DevicesListResponse expectedResponse;

    auto device = expectedResponse.add_devices();
    device->set_id(1);
    device->set_name("foo");
    device->set_type(1);

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
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::CurrentStateResponse>());
    mobilusActor.run();

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::UnexpectedMessage, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, SendRequestTimeouts)
{
    MqttMobilusGtwClientImpl client({ "localhost", 1883, "admin", "admin" });
    MockMqttMobilusActor mobilusActor("localhost", 1883);
    mobilusActor.run();

    ASSERT_TRUE(client.connect());

    proto::DevicesListResponse response;
    auto r = client.sendRequest(proto::DevicesListRequest(), response);

    ASSERT_FALSE(r.has_value());
    ASSERT_EQ(ErrorCode::Timeout, r.error().code());
}

TEST(MqttMobilusGtwClientImplTest, SubscribesMessage)
{
    TestSocketWatcher clientWatcher;
    MqttMobilusGtwClientConfig config = { "localhost", 1883, "admin", "admin" };
    config.clientWatcher = &clientWatcher;

    MqttMobilusGtwClientImpl client(std::move(config));
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    proto::CallEvents expectedCallEvents;
    proto::CallEvents actualCallEvents;

    auto event = expectedCallEvents.add_events();
    event->set_id(1);
    event->set_device_id(2);
    event->set_event_number(3);
    event->set_platform(4);

    mobilusActor.mockResponseFor(MessageType::CallEvents, std::make_unique<proto::CallEvents>(expectedCallEvents));
    mobilusActor.run();

    client.messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, [&](const auto& callEvents) {
        actualCallEvents.CopyFrom(callEvents);
        (void)client.disconnect();
    });

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.send(proto::CallEvents()));

    clientWatcher.loopFor(std::chrono::seconds(1));

    ASSERT_EQ(actualCallEvents.SerializeAsString(), expectedCallEvents.SerializeAsString());
}

TEST(MqttMobilusGtwClientImplTest, SubscribesAllMessages)
{
    TestSocketWatcher clientWatcher;
    MqttMobilusGtwClientConfig config = { "localhost", 1883, "admin", "admin" };
    config.clientWatcher = &clientWatcher;

    MqttMobilusGtwClientImpl client(std::move(config));
    MockMqttMobilusActor mobilusActor("localhost", 1883);

    std::vector<std::unique_ptr<google::protobuf::MessageLite>> subscribedMessages;
    proto::CallEvents expectedCallEvents;

    auto event = expectedCallEvents.add_events();
    event->set_id(1);
    event->set_device_id(2);
    event->set_event_number(3);
    event->set_platform(4);

    proto::DevicesListResponse expectedDeviceList;

    auto device = expectedDeviceList.add_devices();
    device->set_id(1);
    device->set_name("foo");
    device->set_type(1);

    mobilusActor.mockResponseFor(MessageType::DevicesListRequest, std::make_unique<proto::DevicesListResponse>(expectedDeviceList));
    mobilusActor.mockResponseFor(MessageType::CallEvents, std::make_unique<proto::CallEvents>(expectedCallEvents));
    mobilusActor.run();

    client.messageBus().subscribeAll([&](const google::protobuf::MessageLite& message) {
        std::unique_ptr<google::protobuf::MessageLite> subscribedMessage(message.New());

        subscribedMessage->CheckTypeAndMergeFrom(message);
        subscribedMessages.push_back(std::move(subscribedMessage));
    });

    ASSERT_TRUE(client.connect());
    ASSERT_TRUE(client.send(proto::CallEvents()));
    ASSERT_TRUE(client.send(proto::DevicesListRequest()));

    clientWatcher.loopFor(std::chrono::seconds(1));

    ASSERT_EQ(2, subscribedMessages.size());
    ASSERT_EQ(expectedCallEvents.SerializeAsString(), subscribedMessages[0]->SerializeAsString());
}
