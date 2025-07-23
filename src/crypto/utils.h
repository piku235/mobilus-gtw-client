#pragma once

#include "bytes.h"
#include <ctime>

namespace jungi::mobilus_gtw_client::crypto {

bytes timestamp2iv(const time_t timestamp);

}
