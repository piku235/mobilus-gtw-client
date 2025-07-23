#include "MqttMobilusGtwClientImpl.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"

namespace jungi::mobilus_gtw_client {

std::unique_ptr<MqttMobilusGtwClient> MqttMobilusGtwClient::with(MqttMobilusGtwClientConfig config)
{
    return std::make_unique<MqttMobilusGtwClientImpl>(std::move(config));
}

}
