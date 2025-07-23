#pragma once

#include "bytes.h"

namespace jungi::mobilus_gtw_client::crypto {

class Encryptor {
public:
    virtual bytes encrypt(const bytes& plaintext, const bytes& iv) const = 0;
    virtual bytes decrypt(const bytes& ciphertext, const bytes& iv) const = 0;
    virtual ~Encryptor() = default;
};

}
