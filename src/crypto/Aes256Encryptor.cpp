#include "Aes256Encryptor.h"
#include <openssl/evp.h>

namespace jungi::mobilus_gtw_client::crypto {

static const EVP_CIPHER* kCipher = EVP_aes_256_cfb128();

Aes256Encryptor::Aes256Encryptor(bytes key)
    : mKey(std::move(key))
{
}

bytes Aes256Encryptor::encrypt(const bytes& plaintext, const bytes& iv) const
{
    bytes ciphertext(plaintext.size() + EVP_CIPHER_block_size(kCipher));
    int len, totalLen;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx, kCipher, nullptr, mKey.data(), iv.data());
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size());
    totalLen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    totalLen += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(totalLen);

    return ciphertext;
}

bytes Aes256Encryptor::decrypt(const bytes& ciphertext, const bytes& iv) const
{
    bytes plaintext(ciphertext.size() + EVP_CIPHER_block_size(kCipher), '\0');
    int len, totalLen;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx, kCipher, nullptr, mKey.data(), iv.data());
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    EVP_DecryptUpdate(ctx, reinterpret_cast<uint8_t*>(plaintext.data()), &len, reinterpret_cast<const uint8_t*>(ciphertext.data()), ciphertext.size());
    totalLen = len;

    EVP_DecryptFinal_ex(ctx, reinterpret_cast<uint8_t*>(plaintext.data() + len), &len);
    totalLen += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(totalLen);

    return plaintext;
}

}
