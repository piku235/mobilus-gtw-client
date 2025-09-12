#include "SubscribeCommand.h"
#include "jungi/mobilus_gtw_client/MessageType.h"
#include "jungi/mobilus_gtw_client/io/SelectEventLoop.h"
#include "jungi/mobilus_gtw_client/proto/CallEvents.pb.h"
#include "jungi/mobilus_gtw_client/proto/DeviceSettingsRequest.pb.h"

#include <csignal>
#include <iostream>

using namespace jungi::mobilus_gtw_client;

static MqttMobilusGtwClient* sMobilusGtwClient = nullptr;

static void handleSignal(int signal)
{
    if (nullptr != sMobilusGtwClient) {
        (void)sMobilusGtwClient->disconnect();
        sMobilusGtwClient = nullptr;

        std::cout << "disconnected";
    }
}

namespace mobcli::commands {

SubscribeCommand::SubscribeCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int SubscribeCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    io::SelectEventLoop loop;
    auto client = mqttMobilusGtwClient(r, &loop);

    if (!client->connect()) {
        return 1;
    }

    sMobilusGtwClient = client.get();
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    client->messageBus().subscribe<proto::CallEvents>(MessageType::CallEvents, printCallEvents);

    std::cout << "listening..." << std::endl;
    loop.run();

    return 0;
}

void SubscribeCommand::printCallEvents(const proto::CallEvents& callEvents)
{
    std::cout << "CALL EVENTS" << std::endl;

    for (int i = 0; i < callEvents.events_size(); i++) {
        auto& event = callEvents.events(i);

        std::cout << "------" << std::endl
                  << "id: " << (event.has_id() ? event.id() : -1) << std::endl
                  << "device_id: " << (event.has_device_id() ? event.device_id() : -1) << std::endl
                  << "event_number: " << (event.has_event_number() ? event.event_number() : -1) << std::endl
                  << "value(" << event.value().length() << "): " << event.value() << std::endl
                  << "platform: " << (event.has_platform() ? event.platform() : -1) << std::endl
                  << "user: " << (event.has_user() ? event.user() : -1) << std::endl;
    }

    std::cout << std::endl;
}

}
