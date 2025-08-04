#include "SessionCommand.h"

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace jungi::mobilus_gtw_client;

namespace mobcli::commands {

SessionCommand::SessionCommand()
    : mOpts(makeOptions())
{
    addGeneralOptions(mOpts);
}

int SessionCommand::execute(int argc, char* argv[])
{
    auto r = mOpts.parse(argc, argv);

    if (r.count("help")) {
        std::cout << mOpts.help();
        return 0;
    }

    auto client = mqttMobilusGtwClient(r);

    {
        auto expected = client->connect();
        if (!expected) {
            std::cerr << expected.error().message << std::endl;
            return 1;
        }
    }

    std::cout << "user_id: " << client->sessionInfo()->userId << std::endl
              << "admin: " << client->sessionInfo()->admin << std::endl
              << "public_key: " << bin2hex(client->sessionInfo()->publicKey) << std::endl
              << "private_key: " << bin2hex(client->sessionInfo()->privateKey) << std::endl
              << "serial_number: " << client->sessionInfo()->serialNumber << std::endl;

    return 0;
}

std::string SessionCommand::bin2hex(const std::vector<uint8_t>& bytes)
{
    std::ostringstream oss;

    for (size_t i = 0; i < bytes.size(); i++) {
        oss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(bytes[i]);
    }

    return oss.str();
}

}
