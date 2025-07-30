#pragma once

#include "CommonCommand.h"

#include <cxxopts.hpp>

namespace mobcli::commands {

class SessionCommand final : public CommonCommand {
public:
    SessionCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "session"; }
    std::string_view description() const override { return "prints the current session information"; }

private:
    cxxopts::Options mOpts;

    std::string bin2hex(const std::vector<uint8_t>& bytes);
};

}
