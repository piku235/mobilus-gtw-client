#include "TimeUtils.h"

namespace jungi::mobilus_gtw_client {

timeval TimeUtils::milisecondsToTimeval(uint64_t miliseconds)
{
    return {
        static_cast<time_t>(miliseconds / 1000),
        static_cast<suseconds_t>((miliseconds % 1000) * 1000)
    };
}

}
