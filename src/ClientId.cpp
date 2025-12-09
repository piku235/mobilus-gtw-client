#include "ClientId.h"

#include <openssl/crypto.h>
#include <openssl/rand.h>

#include <tuple>

namespace jungi::mobgtw {

ClientId ClientId::unique()
{
    ClientId::value_t value;

    RAND_bytes(value.data(), value.max_size());

    return value;
}

ClientId ClientId::from(value_t value)
{
    return value;
}

ClientId::ClientId(value_t value)
    : mValue(std::move(value))
{
}

const ClientId::value_t& ClientId::value() const
{
    return mValue;
}

std::string ClientId::toHex() const
{
    char hexstr[2 * std::tuple_size<value_t>::value + 1];

    OPENSSL_buf2hexstr_ex(hexstr, sizeof(hexstr), nullptr, static_cast<const unsigned char*>(mValue.data()), mValue.size(), '\0');

    return hexstr;
}

}
