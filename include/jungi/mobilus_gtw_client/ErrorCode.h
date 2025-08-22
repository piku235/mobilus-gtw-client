#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client {

enum class ErrorCode : uint8_t {
    InvalidSession = 1,
    LoginFailed = 2,
    BadResponse = 3,
    BadMessage = 4,
    UnexpectedMessage = 5,
    Transport = 6,
    NoConnection = 7,
    ConnectionTimeout = 8,
    ResponseTimeout = 9,
    Unknown = 255,
};

}
