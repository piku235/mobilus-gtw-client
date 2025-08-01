#include "utils.h"

#include <arpa/inet.h>
#include <openssl/evp.h>

namespace jungi::mobilus_gtw_client::crypto {

bytes timestamp2iv(const time_t timestamp)
{
    bytes iv(EVP_MAX_IV_LENGTH, '\0');
    uint32_t bigEndianTimestamp = htonl(static_cast<uint32_t>(timestamp));
    byte* start = reinterpret_cast<byte*>(&bigEndianTimestamp);

    iv.insert(iv.begin() + (EVP_MAX_IV_LENGTH - sizeof(uint32_t)), start, start + sizeof(uint32_t));

    return iv;
}

}
