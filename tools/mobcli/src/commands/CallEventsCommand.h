#pragma once

#include "CommonCommand.h"
#include <cxxopts.hpp>

namespace mobcli::commands {

class CallEventsCommand final : public CommonCommand {
public:
    CallEventsCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "call-events"; }
    std::string_view description() const override { return "sends call events message to provided device"; }

private:
    cxxopts::Options mOpts;
};

}
