#include "StderrLogger.h"

#include <iostream>

namespace mobcli::mobilus_gtw_client {

void StderrLogger::error(const std::string& message)
{
    std::cerr << message << std::endl;
}

}
