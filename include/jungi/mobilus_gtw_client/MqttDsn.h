#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace jungi::mobilus_gtw_client {

class MqttDsn final {
public:
    static std::optional<MqttDsn> from(const std::string& dsn);
    MqttDsn(std::string host, std::optional<uint16_t> port, std::optional<std::string> username, std::optional<std::string> password, bool secure, std::optional<std::string> cacert);

    const std::string& host() const { return mHost; }
    std::optional<uint16_t> port() const { return mPort; }
    const std::optional<std::string>& username() const { return mUsername; }
    const std::optional<std::string>& password() const { return mPassword; }
    bool isSecure() const { return mSecure; }
    const std::optional<std::string>& cacert() const { return mCacert; }

private:
    std::string mHost;
    std::optional<uint16_t> mPort;
    std::optional<std::string> mUsername;
    std::optional<std::string> mPassword;
    bool mSecure;
    std::optional<std::string> mCacert;
};

}
