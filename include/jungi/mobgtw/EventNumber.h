#pragma once

#include <cstdint>

namespace jungi::mobgtw {

namespace EventNumber {
    constexpr uint8_t User = 1; // ADD, REMOVE, MODIFY
    constexpr uint8_t Device = 2; // ADD, REMOVE, MODIFY
    constexpr uint8_t DeviceGroup = 3; // ADD, REMOVE, MODIFY
    constexpr uint8_t Place = 4; // ADD, REMOVE, MODIFY
    constexpr uint8_t Scene = 5; // ADD, REMOVE, MODIFY
    constexpr uint8_t Triggered = 6; // undescribed, only used on send in mobilus
    constexpr uint8_t Sent = 7;
    constexpr uint8_t Reached = 8;
    constexpr uint8_t Error = 9; // NO_CONNECTION with device or device error
    constexpr uint8_t Session = 10; // session will expire or expired
    constexpr uint8_t RemoteConnection = 11;
    constexpr uint8_t Upgrade = 12;
}

}
