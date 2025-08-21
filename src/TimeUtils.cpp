#include "TimeUtils.h"

namespace jungi::mobilus_gtw_client {

timeval TimeUtils::convertToTimeval(std::chrono::milliseconds milliseconds)
{
    return {
        static_cast<time_t>(milliseconds.count() / 1000),
        static_cast<suseconds_t>((milliseconds.count() % 1000) * 1000)
    };
}

}
