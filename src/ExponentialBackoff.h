#pragma once

#include <chrono>
#include <cstdint>

namespace jungi::mobgtw {

class ExponentialBackoff final {
public:
    ExponentialBackoff(std::chrono::milliseconds baseDelay, std::chrono::milliseconds maxDelay, uint8_t factor = 2);

    void next();
    void reset();
    std::chrono::milliseconds delay() const;

private:
    std::chrono::milliseconds mBaseDelay;
    std::chrono::milliseconds mDelay;
    std::chrono::milliseconds mMaxDelay;
    uint8_t mFactor;
};

}
