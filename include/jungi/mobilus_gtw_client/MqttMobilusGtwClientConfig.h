#pragma once

#include "Error.h"
#include "Envelope.h"
#include "io/NullClientWatcher.h"

#include <google/protobuf/message_lite.h>

#include <cstdint>
#include <string>
#include <memory>
#include <optional>
#include <functional>

namespace jungi::mobilus_gtw_client {

struct MqttMobilusGtwClientConfig final {
    using RawMessageCallback = std::function<void(const Envelope&)>;
    using SessionExpiringCallback = std::function<void(int remaningTime)>;
    using ErrorCallback = std::function<void(const Error& error)>;

    const std::string host;
    const uint32_t port;
    const std::string username;
    const std::string password;
    std::optional<std::string> cafile;
    std::unique_ptr<google::protobuf::MessageLite> keepAliveMessage;
    io::ClientWatcher* clientWatcher = &io::NullClientWatcher::instance();

    /** hooks **/
    SessionExpiringCallback onSessionExpiring = [](int) {};
    RawMessageCallback onRawMessage = [](const Envelope&) {};
    ErrorCallback onError = [](const Error&) {};

    MqttMobilusGtwClientConfig(std::string aHost, uint32_t aPort, std::string aUsername, std::string aPassword, std::optional<std::string> aCafile = std::nullopt)
        : host(std::move(aHost))
        , port(aPort)
        , username(std::move(aUsername))
        , password(std::move(aPassword))
        , cafile(std::move(aCafile))
    {
    }
};

}
