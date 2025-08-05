#pragma once

#include <string>

namespace jungi::mobilus_gtw_client::logging {

class Logger {
public:
    virtual ~Logger() = default;
    virtual void info(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
};

}
