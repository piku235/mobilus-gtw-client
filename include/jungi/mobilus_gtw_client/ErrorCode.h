#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client {

enum class ErrorCode : uint8_t {
    AuthenticationFailed = 1,
    BadResponse = 2,
    BadMessage = 3,
    UnexpectedMessage = 4,
    Transport = 5,
    NoConnection = 6,
    ConnectionTimeout = 7,
    ResponseTimeout = 8,
    Unknown = 255,
};

}
