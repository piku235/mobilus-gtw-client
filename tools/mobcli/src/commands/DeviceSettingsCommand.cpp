#include "DeviceSettingsCommand.h"
#include "jungi/mobilus_gtw_client/Action.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsResponse.pb.h"

#include <iostream>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

DeviceSettingsCommand::DeviceSettingsCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int DeviceSettingsCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    auto client = connectMobilus(r);

    if (!client) {
        return 1;
    }

    proto::DeviceSettingsRequest request;
    proto::DeviceSettingsResponse response;

    request.set_action(Action::Query);

    if (!client->sendRequest(request, response)) {
        std::cerr << "device settings request failed" << std::endl;
        return 1;
    }

    std::cout << "operation_status: " << response.operation_status() << std::endl
              << "latitude: " << response.latitude() << std::endl
              << "longitude: " << response.longitude() << std::endl
              << "remote_access_state: " << response.remote_access_state() << std::endl
              << "remote_access_connection: " << response.remote_access_connection() << std::endl
              << "current_time: " << response.current_time() << std::endl
              << "ethernet_ip: " << (response.has_ethernet_ip() ? response.ethernet_ip() : "?") << std::endl
              << "ethernet_mac: " << response.ethernet_mac() << std::endl
              << "wifi_ip: " << response.wifi_ip() << std::endl
              << "wifi_mac: " << response.wifi_mac() << std::endl
              << "sunrise_time: " << response.sunrise_time() << std::endl
              << "sunset_time: " << response.sunset_time() << std::endl
              << "current_firmware_version: " << response.current_firmware_version() << std::endl
              << "latest_firmware_version: " << response.latest_firmware_version() << std::endl
              << "email_address: " << (response.has_email_address() ? response.email_address() : "?") << std::endl
              << "marketing_materials: " << response.marketing_materials() << std::endl;

    return 0;
}

}
