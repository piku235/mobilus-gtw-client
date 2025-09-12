#pragma once

#include "Command.h"
#include "jungi/mobilus_gtw_client/MqttMobilusGtwClient.h"
#include "jungi/mobilus_gtw_client/io/EventLoop.h"

#include <cxxopts.hpp>
#include <memory>

namespace mobcli::commands {

namespace mobgtw = jungi::mobilus_gtw_client;

class ClientCommonCommand : public Command {
protected:
    void addGeneralOptions(cxxopts::Options& opts);
    std::unique_ptr<mobgtw::logging::Logger> mqttMobilusGtwClientLogger(cxxopts::ParseResult r);
    std::unique_ptr<mobgtw::MqttMobilusGtwClient> mqttMobilusGtwClient(cxxopts::ParseResult r, mobgtw::io::EventLoop* loop = nullptr);
};

}
