#pragma once

#include "bytes.h"
#include <string>

namespace jungi::mobilus_gtw_client::crypto {

bytes sha256(const std::string& text);

}
