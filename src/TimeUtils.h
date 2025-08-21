#pragma once

#include <chrono>
#include <sys/time.h>

namespace jungi::mobilus_gtw_client {

namespace TimeUtils {

    timeval convertToTimeval(std::chrono::milliseconds milliseconds);

}

}
