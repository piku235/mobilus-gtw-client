#pragma once

#include <cstdint>
#include <string>
#include <optional>
#include <unordered_map>

namespace jungi::mobilus_gtw_client {

class MqttDsn final {
public:
    using QueryParams = std::unordered_map<std::string, std::string>;

    static std::optional<MqttDsn> from(const std::string& dsn);
    MqttDsn(std::string host, std::optional<uint16_t> port, std::optional<std::string> username, std::optional<std::string> password, bool secure, QueryParams params);

    const std::string& host() const { return mHost; }
    std::optional<uint16_t> port() const { return mPort; }
    const std::optional<std::string>& username() const { return mUsername; }
    const std::optional<std::string>& password() const { return mPassword; }
    bool isSecure() const { return mSecure; }
    const QueryParams& params() const { return mParams; }

private:
    std::string mHost;
    std::optional<uint16_t> mPort;
    std::optional<std::string> mUsername;
    std::optional<std::string> mPassword;
    bool mSecure;
    QueryParams mParams;
};

}
