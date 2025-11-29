#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client {

enum class ErrorCode : uint8_t {
    InvalidSession = 1,
    NoSession = 2,
    LoginFailed = 3,
    LoginTimeout = 4,
    BadResponse = 5,
    BadMessage = 6,
    UnexpectedMessage = 7,
    Transport = 8,
    NoConnection = 9,
    ConnectionTimeout = 10,
    ConnectionRefused = 11,
    ResponseTimeout = 12,
    Unknown = 255,
};

}
