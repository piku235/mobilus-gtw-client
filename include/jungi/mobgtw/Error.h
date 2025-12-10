#pragma once

#include "ErrorCode.h"

#include <string>

namespace jungi::mobgtw {

class Error final {
public:
    static Error InvalidSession(std::string message) { return { ErrorCode::InvalidSession, std::move(message) }; }
    static Error NoSession(std::string message) { return { ErrorCode::NoSession, std::move(message) }; }
    static Error LoginFailed(std::string message) { return { ErrorCode::LoginFailed, std::move(message) }; }
    static Error LoginTimeout(std::string message) { return { ErrorCode::LoginTimeout, std::move(message) }; }
    static Error InvalidMessage(std::string message) { return { ErrorCode::InvalidMessage, std::move(message) }; }
    static Error BadResponse(std::string message) { return { ErrorCode::BadResponse, std::move(message) }; }
    static Error UnexpectedResponse(std::string message) { return { ErrorCode::UnexpectedResponse, std::move(message) }; }
    static Error ResponseTimeout(std::string message) { return { ErrorCode::ResponseTimeout, std::move(message) }; }
    static Error Transport(std::string message) { return { ErrorCode::Transport, std::move(message) }; }
    static Error NoConnection(std::string message) { return { ErrorCode::NoConnection, std::move(message) }; }
    static Error ConnectionTimeout(std::string message) { return { ErrorCode::ConnectionTimeout, std::move(message) }; }
    static Error ConnectionRefused(std::string message) { return { ErrorCode::ConnectionRefused, std::move(message) }; }

    ErrorCode code() const { return mCode; }
    const std::string& message() const { return mMessage; }

private:
    /* const */ ErrorCode mCode;
    /* const */ std::string mMessage;

    Error(ErrorCode code, std::string message)
        : mCode(code)
        , mMessage(std::move(message))
    {
    }
};

}
