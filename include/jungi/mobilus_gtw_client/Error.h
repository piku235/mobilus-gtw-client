#pragma once

#include "ErrorCode.h"

#include <string>

namespace jungi::mobilus_gtw_client {

class Error final {
public:
    static Error InvalidSession(std::string message) { return { ErrorCode::InvalidSession, std::move(message) }; }
    static Error LoginFailed(std::string message) { return { ErrorCode::LoginFailed, std::move(message) }; }
    static Error BadResponse(std::string message) { return { ErrorCode::BadResponse, std::move(message) }; }
    static Error BadMessage(std::string message) { return { ErrorCode::BadMessage, std::move(message) }; }
    static Error UnexpectedMessage(std::string message) { return { ErrorCode::UnexpectedMessage, std::move(message) }; }
    static Error Transport(std::string message) { return { ErrorCode::Transport, std::move(message) }; }
    static Error NoConnection(std::string message) { return { ErrorCode::NoConnection, std::move(message) }; }
    static Error ConnectionTimeout(std::string message) { return { ErrorCode::ConnectionTimeout, std::move(message) }; }
    static Error ResponseTimeout(std::string message) { return { ErrorCode::ResponseTimeout, std::move(message) }; }
    static Error Unknown(std::string message) { return { ErrorCode::Unknown, std::move(message) }; }

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
