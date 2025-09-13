#include "ClientCommonCommand.h"
#include "Utils.h"
#include "config.h"
#include "jungi/mobilus_gtw_client/MqttDsn.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsRequest.pb.h"
#include "mobilus_gtw_client/StderrLogger.h"

#include <iostream>

using namespace mobcli::mobilus_gtw_client;
using namespace jungi::mobilus_gtw_client;

static const size_t kMobilusMqttPort = 8883;

static void printRawMessage(const Envelope& envelope)
{
    using namespace mobcli::Utils;

    std::cout << "client_id: " << bin2hex(envelope.clientId) << std::endl
              << "message_size: " << envelope.size() << std::endl
              << "message_type: " << static_cast<int>(envelope.messageType) << std::endl
              << "message_body_length: " << envelope.messageBody.size() << std::endl
              << "response_status: " << static_cast<int>(envelope.responseStatus) << std::endl
              << "timestamp: " << envelope.timestamp << std::endl
              << std::endl;
}

namespace mobcli::commands {

void ClientCommonCommand::addGeneralOptions(cxxopts::Options& opts)
{
    // clang-format off
    opts.add_options("General", {
        {"h,host", "mobilus hostname/IP", cxxopts::value<std::string>()->default_value(Utils::getEnvOr("MOBILUS_HOST", ""))},
        {"u,username", "mobilus username/login", cxxopts::value<std::string>()->default_value(Utils::getEnvOr("MOBILUS_USERNAME", "admin"))},
        {"p,password", "mobilus password", cxxopts::value<std::string>()->default_value(Utils::getEnvOr("MOBILUS_PASSWORD", "admin"))},
        {"v,verbose", "prints extra logs", cxxopts::value<bool>()->default_value("false")},
        {"help", "prints help"}
    });
    // clang-format on
}

std::unique_ptr<MqttMobilusGtwClient> ClientCommonCommand::mqttMobilusGtwClient(cxxopts::ParseResult r, io::EventLoop* loop)
{
    static StderrLogger logger;
    auto builder = MqttMobilusGtwClient::builder();

    MqttDsn::QueryParams params;
    params["mobilus_username"] = r["username"].as<std::string>;
    params["mobilus_password"] = r["password"].as<std::string>;
    params["cacert"] = ::kMobilusCaFile;

    builder
        .dsn({ r["host"].as<std::string>(), ::kMobilusMqttPort, {}, {}, true, std::move(params) })
        .useKeepAliveMessage(std::make_unique<proto::DeviceSettingsRequest>())
        .useLogger(&logger);

    if (nullptr != loop) {
        builder.attachTo(loop);
    }

    if (r.count("verbose")) {
        builder.onRawMessage(printRawMessage);
    }

    return builder.build();
}

}
