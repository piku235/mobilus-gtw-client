#pragma once

namespace jungi::mobgtw {

enum class ErrorCode {
    InvalidSession,
    NoSession,
    LoginFailed,
    LoginTimeout,
    InvalidMessage,
    BadResponse,
    UnexpectedResponse,
    ResponseTimeout,
    Transport,
    NoConnection,
    ConnectionTimeout,
    ConnectionRefused,
};

}
