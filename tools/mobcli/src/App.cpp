#include "App.h"
#include "commands/CallEventsCommand.h"
#include "commands/CurrentStateCommand.h"
#include "commands/DeviceListCommand.h"
#include "commands/DeviceSettingsCommand.h"
#include "commands/NetworkSettingsCommand.h"
#include "commands/SessionCommand.h"
#include "commands/SubscribeCommand.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

static const char kConsoleBanner[] = R"(
                    __         ___
   ____ ___  ____  / /_  _____/ (_)
  / __ `__ \/ __ \/ __ \/ ___/ / /
 / / / / / / /_/ / /_/ / /__/ / /
/_/ /_/ /_/\____/_.___/\___/_/_/

Command-line client for interacting with the Mobilus Cosmo GTW
)";

using namespace mobcli::commands;

namespace mobcli {

App::App()
    : mOpts("mobcli <subcommand>", kConsoleBanner)
{
    mOpts.allow_unrecognised_options();
    mOpts.add_options()("help", "prints help");

    registerCommand(std::make_unique<CallEventsCommand>());
    registerCommand(std::make_unique<CurrentStateCommand>());
    registerCommand(std::make_unique<DeviceListCommand>());
    registerCommand(std::make_unique<DeviceSettingsCommand>());
    registerCommand(std::make_unique<NetworkSettingsCommand>());
    registerCommand(std::make_unique<SessionCommand>());
    registerCommand(std::make_unique<SubscribeCommand>());
}

int App::run(int argc, char* argv[])
{
    try {
        auto r = mOpts.parse(argc, argv);

        if (argc < 2) {
            std::cout << help();
            return 0;
        }

        for (auto& [_, cmd] : mCommands) {
            if (!cmd->name().compare(argv[1])) {
                return cmd->execute(argc - 1, argv + 1);
            }
        }

        // must be last
        if (r.count("help")) {
            std::cout << help();
            return 0;
        }

        std::cout << "unknown command: " << argv[1] << std::endl
                  << "type --help to print available commands" << std::endl;

        return 0;
    } catch (const cxxopts::exceptions::parsing& e) {
        std::cout << "invalid args" << std::endl
                  << "type --help to print available args" << std::endl;

        return 1;
    }
}

void App::registerCommand(std::unique_ptr<Command> cmd)
{
    mCommands.emplace(cmd->name(), std::move(cmd));
}

std::string App::help()
{
    uint8_t padding = 0;

    for (auto& [name, cmd] : mCommands) {
        if (name.length() > padding) {
            padding = static_cast<uint8_t>(name.length());
        }
    }

    padding += 2;

    std::ostringstream oss;
    oss << mOpts.help() << std::endl
        << "Commands:" << std::endl;

    for (auto& [_, cmd] : mCommands) {
        oss << "  " << std::left << std::setw(padding) << cmd->name() << cmd->description() << std::endl;
    }

    oss << std::endl;

    return oss.str();
}

}
