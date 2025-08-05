#include "NetworkSettingsCommand.h"
#include "jungi/mobilus_gtw_client/Action.h"
#include "jungi/mobilus_gtw_client/proto/NetworkSettingsRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/NetworkSettingsResponse.pb.h"

#include <iostream>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

NetworkSettingsCommand::NetworkSettingsCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int NetworkSettingsCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    auto client = mqttMobilusGtwClient(r);

    if (auto e = client->connect(); !e) {
        std::cerr << e.error().message() << std::endl;
        return 1;
    }
    
    proto::NetworkSettingsResponse response;
    proto::NetworkSettingsRequest request;

    request.set_action(Action::Query);

    if (auto e = client->sendRequest(request, response); !e) {
        std::cerr << "network settings request failed: " << e.error().message() << std::endl;
        return 1;
    }

    std::cout << "operation_status: " << response.operation_status() << std::endl
              << "ethernet_state: " << response.ethernet_state() << std::endl
              << "wifi_mode: " << response.wifi_mode() << std::endl
              << "wifi_net_name: " << (response.has_wifi_net_name() ? response.wifi_net_name() : "?") << std::endl
              << "wifi_net_password: " << (response.has_wifi_net_password() ? response.wifi_net_password() : "?") << std::endl;

    return 0;
}

}
