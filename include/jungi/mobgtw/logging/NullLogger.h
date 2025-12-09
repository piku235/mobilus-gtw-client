#pragma once

#include "Logger.h"

namespace jungi::mobgtw::logging {

class NullLogger final : public Logger {
public:
    static NullLogger& instance()
    {
        static NullLogger logger;

        return logger;
    }

    void info(const std::string&) override { }
    void error(const std::string&) override { }

private:
    NullLogger() = default;
};

}
