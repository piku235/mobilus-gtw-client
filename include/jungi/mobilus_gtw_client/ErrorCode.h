#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client {

enum class ErrorCode : uint8_t {
    InvalidSession = 1,
    LoginFailed = 2,
    LoginTimeout = 3,
    BadResponse = 4,
    BadMessage = 5,
    UnexpectedMessage = 6,
    Transport = 7,
    NoConnection = 8,
    ConnectionTimeout = 9,
    ConnectionRefused = 10,
    ResponseTimeout = 11,
    Unknown = 255,
};

}
