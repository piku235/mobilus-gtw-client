#pragma once

#include <string>

namespace jungi::mobilus_gtw_client {

struct Error final {
public:    
    enum class Code {
        AuthenticationFailed,
        BadResponse,
        Transport,
        NoConnection,
        Unknown,
    };

    const Code code;
    const std::string message;

    static Error AuthenticationFailed(std::string message) { return { Code::AuthenticationFailed, std::move(message) }; }
    static Error BadResponse(std::string message) { return { Code::BadResponse, std::move(message) }; }
    static Error Transport(std::string message) { return { Code::Transport, std::move(message) }; }
    static Error NoConnection(std::string message) { return { Code::NoConnection, std::move(message) }; }
    static Error Unknown(std::string message) { return { Code::Unknown, std::move(message) }; }

private:
    Error(Code aCode, std::string aMessage): code(aCode), message(std::move(aMessage)) {}
};

}
