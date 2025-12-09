#pragma once

#include "bytes.h"

namespace jungi::mobgtw::crypto {

class Encryptor {
public:
    virtual ~Encryptor() = default;

    virtual bytes encrypt(const bytes& plaintext, const bytes& key, const bytes& iv) const = 0;
    virtual bytes decrypt(const bytes& ciphertext, const bytes& key, const bytes& iv) const = 0;
};

}
