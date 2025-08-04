#pragma once

#include "ClientCommonCommand.h"
#include "jungi/mobilus_gtw_client/proto/Event.pb.h"

#include <cxxopts.hpp>

namespace mobcli::commands {

class CurrentStateCommand final : public ClientCommonCommand {
public:
    CurrentStateCommand();

    int execute(int argc, char* argv[]) override;
    std::string_view name() const override { return "current-state"; }
    std::string_view description() const override { return "makes current state query and prints the response"; }

private:
    cxxopts::Options mOpts;
};

}
