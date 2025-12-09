#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace jungi::mobgtw {

struct MqttDsn final {
    bool secure;
    std::optional<std::string> username;
    std::optional<std::string> password;
    std::string host;
    std::optional<uint16_t> port;
    std::optional<std::string> cacert;
    std::optional<bool> verify;

    static std::optional<MqttDsn> from(const std::string& dsn);
};

}
