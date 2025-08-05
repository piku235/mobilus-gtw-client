#include "CallEventsCommand.h"
#include "jungi/mobilus_gtw_client/EventNumber.h"
#include "jungi/mobilus_gtw_client/Platform.h"
#include "jungi/mobilus_gtw_client/proto/CallEvents.pb.h"

#include <iostream>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

CallEventsCommand::CallEventsCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);

    // clang-format off
    mOpts.add_options("Command", {
        {"device-id", "mobilus device id", cxxopts::value<int64_t>()},
        {"value", "event value", cxxopts::value<std::string>()},
        {"platform", "platform that sends the event", cxxopts::value<int32_t>()->default_value(std::to_string(Platform::Web))}
    });
    // clang-format on
}

int CallEventsCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (argc < 2 || r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    if (!r["device-id"].count() || !r["value"].count()) {
        std::cerr << "device-id and value are required" << std::endl;
        return 1;
    }

    auto client = mqttMobilusGtwClient(r);

    if (auto e = client->connect(); !e) {
        std::cerr << e.error().message() << std::endl;
        return 1;
    }

    proto::CallEvents callEvents;

    auto event = callEvents.add_events();
    event->set_device_id(r["device-id"].as<int64_t>());
    event->set_event_number(static_cast<int32_t>(EventNumber::Triggered));
    event->set_value(r["value"].as<std::string>());
    event->set_platform(r["platform"].as<int32_t>());

    auto expected = client->send(callEvents);
    if (!expected) {
        std::cerr << "call events failed: " << expected.error().message() << std::endl;
        return 1;
    }

    return 0;
}

}
