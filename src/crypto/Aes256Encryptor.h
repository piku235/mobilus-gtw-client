#pragma once

#include "Encryptor.h"

namespace jungi::mobilus_gtw_client::crypto {

class Aes256Encryptor : public Encryptor {
public:
    explicit Aes256Encryptor(bytes key);
    bytes encrypt(const bytes& plaintext, const bytes& iv) const;
    bytes decrypt(const bytes& ciphertext, const bytes& iv) const;

private:
    const bytes mKey;
};

}
