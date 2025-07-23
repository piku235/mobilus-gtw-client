#pragma once

#include "CommonCommand.h"

#include <cxxopts.hpp>

namespace mobcli::commands {

class NetworkSettingsCommand final : public CommonCommand {
public:
    NetworkSettingsCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "network-settings"; }
    std::string_view description() const override { return "makes network settings query and prints the response"; }

private:
    cxxopts::Options mOpts;
};

}
