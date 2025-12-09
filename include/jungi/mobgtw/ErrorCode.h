#pragma once

#include <cstdint>

namespace jungi::mobgtw {

enum class ErrorCode : uint8_t {
    InvalidSession = 1,
    NoSession = 2,
    LoginFailed = 3,
    LoginTimeout = 4,
    InvalidMessage = 5,
    BadResponse = 6,
    UnexpectedResponse = 7,
    ResponseTimeout = 8,
    Transport = 9,
    NoConnection = 10,
    ConnectionTimeout = 11,
    ConnectionRefused = 12,
    Unknown = 255,
};

}
