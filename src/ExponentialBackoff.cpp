#include "ExponentialBackoff.h"

#include <algorithm>
#include <cstdlib>

namespace jungi::mobgtw {

ExponentialBackoff::ExponentialBackoff(std::chrono::milliseconds baseDelay, std::chrono::milliseconds maxDelay, uint8_t factor)
    : mBaseDelay(baseDelay)
    , mDelay(baseDelay)
    , mMaxDelay(maxDelay)
    , mFactor(factor)
{
}

void ExponentialBackoff::next()
{
    mDelay = std::min(mDelay * mFactor, mMaxDelay);
}

void ExponentialBackoff::reset()
{
    mDelay = mBaseDelay;
}

std::chrono::milliseconds ExponentialBackoff::delay() const
{
    return std::chrono::milliseconds(rand() % mDelay.count());
}

}
