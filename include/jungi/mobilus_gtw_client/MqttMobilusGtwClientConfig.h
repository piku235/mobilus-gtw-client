#pragma once

#include "Envelope.h"
#include "Error.h"
#include "io/NullClientWatcher.h"
#include "logging/NullLogger.h"

#include <google/protobuf/message_lite.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace jungi::mobilus_gtw_client {

struct MqttMobilusGtwClientConfig final {
    using RawMessageCallback = std::function<void(const Envelope&)>;
    using SessionExpiringCallback = std::function<void(int remaningTime)>;

    const std::string host;
    const uint32_t port;
    const std::string username;
    const std::string password;
    std::optional<std::string> cafile;
    std::unique_ptr<google::protobuf::MessageLite> keepAliveMessage;
    io::ClientWatcher* clientWatcher = &io::NullClientWatcher::instance();
    logging::Logger* logger = &logging::NullLogger::instance();
    std::chrono::milliseconds connectTimeout = std::chrono::seconds(1);
    std::chrono::milliseconds responseTimeout = std::chrono::seconds(5);

    /** hooks **/
    SessionExpiringCallback onSessionExpiring = [](int) {};
    RawMessageCallback onRawMessage = [](const Envelope&) {};

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
