#pragma once

#include <cstdint>

namespace jungi::mobilus_gtw_client::io {

class SocketEvents final {
public:
    enum Event : uint8_t { 
        Read = 0x1,
        Write = 0x2,
    };

    void set(Event event) { mMask |= static_cast<uint8_t>(event); }
    void unset(Event event) { mMask &= ~static_cast<uint8_t>(event); }
    bool has(Event event) const { return mMask & static_cast<uint8_t>(event); }
private:
    uint8_t mMask = 0;
};

}
