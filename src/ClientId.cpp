#include "jungi/mobilus_gtw_client/ClientId.h"

#include <openssl/rand.h>
#include <openssl/crypto.h>

namespace jungi::mobilus_gtw_client {

ClientId ClientId::unique()
{
    ClientId clientId {};

    RAND_bytes(clientId.mValue, clientId.size());

    return clientId;
}

std::string ClientId::toHex() const
{
    char hexstr[2 * sizeof(mValue) + 1];

    OPENSSL_buf2hexstr_ex(hexstr, sizeof(hexstr), nullptr, static_cast<const unsigned char*>(mValue), sizeof(mValue), '\0');

    return hexstr;
}

}
