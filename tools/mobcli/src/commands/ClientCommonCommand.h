#pragma once

#include "Command.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/io/ClientWatcher.h"

#include <cxxopts.hpp>
#include <memory>

namespace mobcli::commands {

class ClientCommonCommand : public Command {
protected:
    void addGeneralOptions(cxxopts::Options& opts);
    std::unique_ptr<jungi::mobilus_gtw_client::MqttMobilusGtwClient> mqttMobilusGtwClient(cxxopts::ParseResult r, jungi::mobilus_gtw_client::io::ClientWatcher* clientWatcher = nullptr);
};

}
