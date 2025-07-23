#pragma once

#include "CommonCommand.h"

#include <cxxopts.hpp>

namespace mobcli::commands {

class DeviceSettingsCommand final : public CommonCommand {
public:
    DeviceSettingsCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "device-settings"; }
    std::string_view description() const override { return "makes device settings query and prints the response"; }

private:
    cxxopts::Options mOpts;
};

}
