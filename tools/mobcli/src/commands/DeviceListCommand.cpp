#include "DeviceListCommand.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/DevicesListResponse.pb.h"

#include <iostream>
#include <sstream>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

DeviceListCommand::DeviceListCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int DeviceListCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    auto client = mqttMobilusGtwClient(r);

    if (!client->connect()) {
        return 1;
    }

    proto::DevicesListResponse response;
        
    if (!client->sendRequest(proto::DevicesListRequest(), response)) {
        return 1;
    }

    std::cout << "devices_count: " << response.devices_size() << std::endl;

    for (int i = 0; i < response.devices_size(); i++) {
        auto& device = response.devices(i);

        std::cout << "------" << std::endl
                  << "id: " << (device.has_id() ? device.id() : -1) << std::endl
                  << "name: " << device.name() << std::endl
                  << "type: " << (device.has_type() ? device.type() : -1) << std::endl
                  << "icon: " << (device.has_icon() ? device.icon() : -1) << std::endl
                  << "inserttime: " << device.inserttime() << std::endl
                  << "assigned_place_ids: " << (device.has_assigned_place_ids() ? device.assigned_place_ids() : -1) << std::endl
                  << "assigned_group_ids: " << formatList(device.assigned_group_ids()) << std::endl;
    }

    return 0;
}

template <typename T>
std::string DeviceListCommand::formatList(const google::protobuf::RepeatedField<T>& repeatedField)
{
    std::ostringstream oss;

    oss << "[";

    for (int i = 0; i < repeatedField.size(); i++) {
        oss << repeatedField.Get(i);

        if (i != repeatedField.size() - 1) {
            oss << ", ";
        }
    }

    oss << "]";

    return oss.str();
}

}
