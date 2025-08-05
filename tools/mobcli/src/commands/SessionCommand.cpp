#include "Utils.h"
#include "SessionCommand.h"

#include <cstdint>
#include <iostream>

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

    if (auto e = client->connect(); !e) {
        std::cerr << e.error().message() << std::endl;
        return 1;
    }

    std::cout << "user_id: " << client->sessionInfo()->userId << std::endl
              << "admin: " << client->sessionInfo()->admin << std::endl
              << "public_key: " << Utils::bin2hex(client->sessionInfo()->publicKey) << std::endl
              << "private_key: " << Utils::bin2hex(client->sessionInfo()->privateKey) << std::endl
              << "serial_number: " << client->sessionInfo()->serialNumber << std::endl;

    return 0;
}

}
