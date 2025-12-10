#pragma once

#include <cstdint>

namespace jungi::mobgtw {

namespace MessageType {
    constexpr uint8_t LoginRequest = 1;
    constexpr uint8_t LoginResponse = 2;
    constexpr uint8_t DevicesListRequest = 3;
    constexpr uint8_t DevicesListResponse = 4;
    constexpr uint8_t CallEvents = 13;
    constexpr uint8_t UpdateDeviceRequest = 24;
    constexpr uint8_t UpdateDeviceResponse = 25;
    constexpr uint8_t CurrentStateRequest = 26;
    constexpr uint8_t CurrentStateResponse = 27;
    constexpr uint8_t DeviceSettingsRequest = 28;
    constexpr uint8_t DeviceSettingsResponse = 29;
    constexpr uint8_t NetworkSettingsRequest = 30;
    constexpr uint8_t NetworkSettingsResponse = 31;
    constexpr uint8_t Unknown = 255;
}

}
