#pragma once

#include "Command.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/io/ClientWatcher.h"

#include <cxxopts.hpp>

#include <cstdlib>
#include <memory>

namespace mobcli::commands {

class CommonCommand : public Command {
protected:
    void addGeneralOptions(cxxopts::Options& opts);
    const char* getEnvOr(const char* name, const char* defaultValue);
    std::unique_ptr<jungi::mobilus_gtw_client::MqttMobilusGtwClient> mqttMobilusGtwClient(cxxopts::ParseResult r, jungi::mobilus_gtw_client::io::ClientWatcher* clientWatcher = nullptr);
};

}
