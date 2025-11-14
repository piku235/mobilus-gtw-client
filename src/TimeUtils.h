#pragma once

#include <chrono>
#include <sys/time.h>

namespace jungi::mobilus_gtw_client {

namespace TimeUtils {

    template <class Rep, class Period>
    timeval convertToTimeval(std::chrono::duration<Rep, Period> duration)
    {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto microsecs = std::chrono::duration_cast<std::chrono::microseconds>(duration - secs);

        return {
            static_cast<time_t>(secs.count()),
            static_cast<suseconds_t>(microsecs.count())
        };
    }

}

}
