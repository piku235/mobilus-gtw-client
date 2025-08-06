#pragma once

#include "ClientCommonCommand.h"

#include <cxxopts.hpp>

namespace jungi::mobilus_gtw_client::proto {
class CallEvents;
}

namespace mobcli::commands {

class SubscribeCommand final : public ClientCommonCommand {
public:
    SubscribeCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "sub"; }
    std::string_view description() const override { return "subscribes to events"; }

private:
    cxxopts::Options mOpts;

    static void printCallEvents(const jungi::mobilus_gtw_client::proto::CallEvents& callEvents);
};

}
