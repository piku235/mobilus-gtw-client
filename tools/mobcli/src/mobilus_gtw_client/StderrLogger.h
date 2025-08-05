#pragma once

#include "jungi/mobilus_gtw_client/logging/Logger.h"

namespace mobcli::mobilus_gtw_client {

class StderrLogger final : public jungi::mobilus_gtw_client::logging::Logger {
public:
    void info(const std::string& message) override {}
    void error(const std::string& message) override;
};

}
