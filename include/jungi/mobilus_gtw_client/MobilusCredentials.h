#pragma once

#include <string>

namespace jungi::mobilus_gtw_client {

struct MobilusCredentials final {
    std::string username;
    std::string password;

    MobilusCredentials(std::string aUsername, std::string aPassword)
        : username(std::move(aUsername))
        , password(std::move(aPassword))
    {
    }
};

}
