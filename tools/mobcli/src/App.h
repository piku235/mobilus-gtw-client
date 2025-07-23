#pragma once

#include "Command.h"

#include <cxxopts.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace mobcli {

class App final {
public:
    App();
    int run(int argc, char* argv[]);

private:
    cxxopts::Options mOpts;
    std::unordered_map<std::string, std::unique_ptr<Command>> mCommands;

    void registerCommand(std::unique_ptr<Command> cmd);
    std::string help();
};

}
