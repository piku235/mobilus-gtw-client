#include "jungi/mobilus_gtw_client/MqttDsn.h"

#include <gtest/gtest.h>

using jungi::mobilus_gtw_client::MqttDsn;

TEST(MqttDsnTest, ParsesDsnString)
{
    auto mqttDsn = MqttDsn::from("mqtt://user:pass@127.0.0.1:1883?foo=bar&cacert=ca.cert");

    ASSERT_TRUE(mqttDsn.has_value());
    ASSERT_FALSE(mqttDsn->secure);
    ASSERT_TRUE(mqttDsn->username.has_value());
    ASSERT_EQ("user", *mqttDsn->username);
    ASSERT_TRUE(mqttDsn->password.has_value());
    ASSERT_EQ("pass", *mqttDsn->password);
    ASSERT_EQ("127.0.0.1", mqttDsn->host);
    ASSERT_TRUE(mqttDsn->port.has_value());
    ASSERT_EQ(1883, *mqttDsn->port);
    ASSERT_TRUE(mqttDsn->cacert.has_value());
    ASSERT_EQ("ca.cert", *mqttDsn->cacert);

    auto mqttsDsn = MqttDsn::from("mqtts://localhost");

    ASSERT_TRUE(mqttsDsn.has_value());
    ASSERT_TRUE(mqttsDsn->secure);
    ASSERT_FALSE(mqttsDsn->username.has_value());
    ASSERT_FALSE(mqttsDsn->password.has_value());
    ASSERT_EQ("localhost", mqttsDsn->host);
    ASSERT_FALSE(mqttsDsn->port.has_value());
    ASSERT_FALSE(mqttsDsn->cacert.has_value());
}

TEST(MqttDsnTest, ParseDsnStringFails)
{
    auto mqttDsn = MqttDsn::from("http://user:pass@127.0.0.1:1883?foo=bar");

    ASSERT_FALSE(mqttDsn.has_value());
}
