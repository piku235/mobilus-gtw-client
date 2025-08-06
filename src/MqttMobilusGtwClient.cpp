#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "MqttMobilusGtwClientImpl.h"

namespace jungi::mobilus_gtw_client {

std::unique_ptr<MqttMobilusGtwClient> MqttMobilusGtwClient::with(MqttMobilusGtwClientConfig config)
{
    return std::make_unique<MqttMobilusGtwClientImpl>(std::move(config));
}

}
