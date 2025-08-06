#pragma once

#include <cstdio>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

namespace mobcli {

namespace Utils {

    inline const char* getEnvOr(const char* name, const char* defaultValue)
    {
        auto value = getenv(name);

        if (nullptr == value) {
            return defaultValue;
        }

        return value;
    }

    template <class TContainer>
    static std::string bin2hex(const TContainer& buf)
    {
        std::ostringstream oss;

        for (auto it = std::begin(buf); it != std::end(buf); it++) {
            oss << std::setw(2) << std::setfill('0') << std::hex << std::uppercase << static_cast<int>(*it);
        }

        return oss.str();
    }

}

}
