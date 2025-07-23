#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace jungi::mobilus_gtw_client {

struct SessionInformation final {
    int64_t userId;
    bool admin;
    std::vector<uint8_t> publicKey;
    std::vector<uint8_t> privateKey;
    std::string serialNumber;
};

}
