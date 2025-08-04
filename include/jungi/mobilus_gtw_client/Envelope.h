#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <optional>

namespace jungi::mobilus_gtw_client {

struct Envelope {
    struct ResponseStatus {
        static constexpr uint8_t Success = 0;
        static constexpr uint8_t AuthenticationFailed = 1;
    };

    uint8_t messageType;
    uint32_t timestamp;
    std::array<uint8_t, 6> clientId;
    uint8_t platform;
    uint8_t responseStatus;
    std::vector<uint8_t> messageBody;

    static std::optional<Envelope> deserialize(const uint8_t* payload, uint32_t size);
    std::vector<uint8_t> serialize() const;

    uint32_t size() const;
};

}
