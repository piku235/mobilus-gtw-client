#pragma once

#include <string>

namespace jungi::mobilus_gtw_client {

class ClientId {
public:
    static ClientId unique();
    uint8_t* data() { return mValue; }
    const uint8_t* data() const { return mValue; }
    constexpr uint8_t size() const { return sizeof(mValue); }
    const uint8_t* begin() const { return mValue; }
    const uint8_t* end() const { return mValue + size(); }
    std::string toHex() const;

private:
    uint8_t mValue[6];
};

}
