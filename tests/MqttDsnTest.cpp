#include "jungi/mobilus_gtw_client/MqttDsn.h"

#include <gtest/gtest.h>

using jungi::mobilus_gtw_client::MqttDsn;

TEST(MqttDsnTest, ParsesDsnString)
{
    auto mqttsDsn = MqttDsn::from("mqtts://user:pass@127.0.0.1:8883?foo=bar&cacert=/etc/certs/ca.cert&verify=true");

    ASSERT_TRUE(mqttsDsn.has_value());
    ASSERT_TRUE(mqttsDsn->secure);
    ASSERT_TRUE(mqttsDsn->username.has_value());
    ASSERT_EQ("user", *mqttsDsn->username);
    ASSERT_TRUE(mqttsDsn->password.has_value());
    ASSERT_EQ("pass", *mqttsDsn->password);
    ASSERT_EQ("127.0.0.1", mqttsDsn->host);
    ASSERT_TRUE(mqttsDsn->port.has_value());
    ASSERT_EQ(8883, *mqttsDsn->port);
    ASSERT_TRUE(mqttsDsn->cacert.has_value());
    ASSERT_EQ("/etc/certs/ca.cert", *mqttsDsn->cacert);
    ASSERT_TRUE(mqttsDsn->verify.has_value());
    ASSERT_TRUE(*mqttsDsn->verify);

    auto verifyFalseDsn = MqttDsn::from("mqtts://127.0.0.1:8884?verify=false");

    ASSERT_TRUE(verifyFalseDsn.has_value());
    ASSERT_TRUE(verifyFalseDsn->secure);
    ASSERT_FALSE(verifyFalseDsn->username.has_value());
    ASSERT_FALSE(verifyFalseDsn->password.has_value());
    ASSERT_EQ("127.0.0.1", verifyFalseDsn->host);
    ASSERT_TRUE(verifyFalseDsn->port.has_value());
    ASSERT_EQ(8884, *verifyFalseDsn->port);
    ASSERT_FALSE(verifyFalseDsn->cacert.has_value());
    ASSERT_TRUE(verifyFalseDsn->verify.has_value());
    ASSERT_FALSE(*verifyFalseDsn->verify);

    auto verifyInvalidDsn = MqttDsn::from("mqtts://127.0.0.1:8884?verify=invalid");

    ASSERT_TRUE(verifyInvalidDsn.has_value());
    ASSERT_TRUE(verifyInvalidDsn->secure);
    ASSERT_FALSE(verifyInvalidDsn->username.has_value());
    ASSERT_FALSE(verifyInvalidDsn->password.has_value());
    ASSERT_EQ("127.0.0.1", verifyInvalidDsn->host);
    ASSERT_TRUE(verifyInvalidDsn->port.has_value());
    ASSERT_EQ(8884, *verifyInvalidDsn->port);
    ASSERT_FALSE(verifyInvalidDsn->cacert.has_value());
    ASSERT_FALSE(verifyInvalidDsn->verify.has_value());

    auto mqttDsn = MqttDsn::from("mqtt://localhost");

    ASSERT_TRUE(mqttDsn.has_value());
    ASSERT_FALSE(mqttDsn->secure);
    ASSERT_FALSE(mqttDsn->username.has_value());
    ASSERT_FALSE(mqttDsn->password.has_value());
    ASSERT_EQ("localhost", mqttDsn->host);
    ASSERT_FALSE(mqttDsn->port.has_value());
    ASSERT_FALSE(mqttDsn->cacert.has_value());
    ASSERT_FALSE(mqttDsn->verify.has_value());
}

TEST(MqttDsnTest, ParseDsnStringFails)
{
    auto emptyDsn = MqttDsn::from("");
    ASSERT_FALSE(emptyDsn.has_value());

    auto invalidDsn = MqttDsn::from("invalid");
    ASSERT_FALSE(emptyDsn.has_value());

    auto httpDsn = MqttDsn::from("http://user:pass@127.0.0.1:1883?foo=bar");
    ASSERT_FALSE(httpDsn.has_value());

    auto invalidPort = MqttDsn::from("mqtt://localhost:qwerty");
    ASSERT_FALSE(invalidPort.has_value());
}
