#pragma once

#include "bytes.h"
#include <string>

namespace jungi::mobgtw::crypto {

bytes sha256(const std::string& text);

}
