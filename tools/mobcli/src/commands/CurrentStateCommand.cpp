#include "CurrentStateCommand.h"
#include "jungi/mobilus_gtw_client/proto/CurrentStateRequest.pb.h"
#include "jungi/mobilus_gtw_client/proto/CurrentStateResponse.pb.h"

#include <iostream>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

CurrentStateCommand::CurrentStateCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int CurrentStateCommand::execute(int argc, char* argv[])
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

    proto::CurrentStateResponse response;

    if (!client->sendRequest(proto::CurrentStateRequest(), response)) {
        return 1;
    }

    std::cout << "events_size: " << response.events_size() << std::endl;

    for (int i = 0; i < response.events_size(); i++) {
        auto& event = response.events(i);

        std::cout << "------" << std::endl
                  << "id: " << (event.has_id() ? event.id() : -1) << std::endl
                  << "device_id: " << (event.has_device_id() ? event.device_id() : -1) << std::endl
                  << "event_number: " << (event.has_event_number() ? event.event_number() : -1) << std::endl
                  << "value(" << event.value().length() << "): " << event.value() << std::endl
                  << "platform: " << (event.has_platform() ? event.platform() : -1) << std::endl
                  << "user: " << (event.has_user() ? event.user() : -1) << std::endl;
    }

    return 0;
}

}
