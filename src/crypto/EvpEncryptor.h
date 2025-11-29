#pragma once

#include "Encryptor.h"

#include <openssl/evp.h>

namespace jungi::mobilus_gtw_client::crypto {

class EvpEncryptor final : public Encryptor {
public:
    static EvpEncryptor Aes256Cfb128() { return { EVP_aes_256_cfb128() }; }

    bytes encrypt(const bytes& plaintext, const bytes& key, const bytes& iv) const;
    bytes decrypt(const bytes& ciphertext, const bytes& key, const bytes& iv) const;

private:
    const EVP_CIPHER* mCipher;

    EvpEncryptor(const EVP_CIPHER* cipher);
};

}
