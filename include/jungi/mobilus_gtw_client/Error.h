#pragma once

#include <cstdint>
#include <string>

namespace jungi::mobilus_gtw_client {

class Error final {
public:    
    enum class Code: uint8_t {
        AuthenticationFailed = 0,
        BadResponse = 1,
        BadMessage = 2,
        UnexpectedMessage = 3,
        Transport = 4,
        NoConnection = 5,
        Unknown = 255,
    };

    static Error AuthenticationFailed(std::string message) { return { Code::AuthenticationFailed, std::move(message) }; }
    static Error BadResponse(std::string message) { return { Code::BadResponse, std::move(message) }; }
    static Error BadMessage(std::string message) { return { Code::BadMessage, std::move(message) }; }
    static Error UnexpectedMessage(std::string message) { return { Code::UnexpectedMessage, std::move(message) }; }
    static Error Transport(std::string message) { return { Code::Transport, std::move(message) }; }
    static Error NoConnection(std::string message) { return { Code::NoConnection, std::move(message) }; }
    static Error Unknown(std::string message) { return { Code::Unknown, std::move(message) }; }

    Code code() const { return mCode; }
    const std::string& message() const { return mMessage; }

private:
    /* const */ Code mCode;
    /* const */ std::string mMessage;

    Error(Code code, std::string message): mCode(code), mMessage(std::move(message)) {}
};

}
