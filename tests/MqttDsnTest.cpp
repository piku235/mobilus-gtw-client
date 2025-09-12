#include "jungi/mobilus_gtw_client/MqttDsn.h"

#include <gtest/gtest.h>

using jungi::mobilus_gtw_client::MqttDsn;

TEST(MqttDsnTest, ParsesDsnString)
{
    auto mqttDsn = MqttDsn::from("mqtt://user:pass@127.0.0.1:1883?foo=bar&baz=zoo");

    ASSERT_TRUE(mqttDsn.has_value());
    ASSERT_FALSE(mqttDsn->isSecure());
    ASSERT_TRUE(mqttDsn->username().has_value());
    ASSERT_EQ("user", *mqttDsn->username());
    ASSERT_TRUE(mqttDsn->password().has_value());
    ASSERT_EQ("pass", *mqttDsn->password());
    ASSERT_EQ("127.0.0.1", mqttDsn->host());
    ASSERT_TRUE(mqttDsn->port().has_value());
    ASSERT_EQ(1883, *mqttDsn->port());
    ASSERT_EQ(2, mqttDsn->params().size());
    ASSERT_EQ(1, mqttDsn->params().count("foo"));
    ASSERT_EQ("bar", mqttDsn->params().at("foo"));
    ASSERT_EQ(1, mqttDsn->params().count("baz"));
    ASSERT_EQ("zoo", mqttDsn->params().at("baz"));

    auto mqttsDsn = MqttDsn::from("mqtts://localhost");

    ASSERT_TRUE(mqttsDsn.has_value());
    ASSERT_TRUE(mqttsDsn->isSecure());
    ASSERT_FALSE(mqttsDsn->username().has_value());
    ASSERT_FALSE(mqttsDsn->password().has_value());
    ASSERT_EQ("localhost", mqttsDsn->host());
    ASSERT_FALSE(mqttsDsn->port().has_value());
    ASSERT_TRUE(mqttsDsn->params().empty());
}

TEST(MqttDsnTest, ParseDsnStringFails)
{
    auto mqttDsn = MqttDsn::from("http://user:pass@127.0.0.1:1883?foo=bar");

    ASSERT_FALSE(mqttDsn.has_value());
}
