#include "CommonCommand.h"
#include "config.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsRequest.pb.h"

#include <iostream>

using namespace jungi::mobilus_gtw_client;

static const size_t kMobilusMqttPort = 8883;

static void printRawMessage(const Envelope& envelope)
{
    std::cout << "client_id: " << envelope.clientId.toHex() << std::endl
              << "message_size: " << envelope.size() << std::endl
              << "message_type: " << static_cast<int>(envelope.messageType) << std::endl
              << "message_body_length: " << envelope.messageBody.size() << std::endl
              << "response_status: " << static_cast<int>(envelope.responseStatus) << std::endl
              << "timestamp: " << envelope.timestamp << std::endl
              << std::endl;
}

namespace mobcli::commands {

void CommonCommand::addGeneralOptions(cxxopts::Options& opts)
{
    // clang-format off
    opts.add_options("General", {
        {"h,host", "mobilus hostname/IP", cxxopts::value<std::string>()->default_value(getEnvOr("MOBILUS_HOST", ""))},
        {"u,username", "mobilus username/login", cxxopts::value<std::string>()->default_value(getEnvOr("MOBILUS_USERNAME", "admin"))},
        {"p,password", "mobilus password", cxxopts::value<std::string>()->default_value(getEnvOr("MOBILUS_PASSWORD", "admin"))},
        {"v,verbose", "prints extra logs", cxxopts::value<bool>()->default_value("false")},
        {"help", "prints help"}
    });
    // clang-format on
}

const char* CommonCommand::getEnvOr(const char* name, const char* defaultValue)
{
    auto value = getenv(name);

    if (nullptr == value) {
        return defaultValue;
    }

    return value;
}

std::unique_ptr<MqttMobilusGtwClient> CommonCommand::connectMobilus(cxxopts::ParseResult r, io::ClientWatcher* clientWatcher)
{
    MqttMobilusGtwClientConfig config(r["host"].as<std::string>(), kMobilusMqttPort, r["username"].as<std::string>(), r["password"].as<std::string>(), ::kMobilusCaFile);
    
    config.keepAliveMessage = std::make_unique<proto::DeviceSettingsRequest>();

    if (nullptr != clientWatcher) {
        config.clientWatcher = clientWatcher;
    }

    if (r.count("verbose")) {
        config.onRawMessage = printRawMessage;
    }

    auto client = MqttMobilusGtwClient::with(std::move(config));

    if (!client->connect()) {
        std::cerr << "connection failed" << std::endl;
        return nullptr;
    }

    return client;
}

}
