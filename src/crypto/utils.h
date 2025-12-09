#pragma once

#include "bytes.h"
#include <ctime>

namespace jungi::mobgtw::crypto {

bytes timestamp2iv(const time_t timestamp);

}
