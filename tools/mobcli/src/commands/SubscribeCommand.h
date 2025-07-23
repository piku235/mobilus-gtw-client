#pragma once

#include "CommonCommand.h"

#include <cxxopts.hpp>

namespace mobcli::commands {

class SubscribeCommand final : public CommonCommand {
public:
    SubscribeCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "sub"; }
    std::string_view description() const override { return "subscribes to events"; }

private:
    cxxopts::Options mOpts;
};

}
