#pragma once

#include <cstdint>
#include <sys/time.h>

namespace jungi::mobilus_gtw_client {

namespace TimeUtils {

    timeval milisecondsToTimeval(uint64_t miliseconds);

}

}
