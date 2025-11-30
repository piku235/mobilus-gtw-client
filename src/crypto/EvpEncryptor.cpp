#include "EvpEncryptor.h"

namespace jungi::mobilus_gtw_client::crypto {

EvpEncryptor::EvpEncryptor(const EVP_CIPHER* cipher)
    : mCipher(cipher)
{
}

bytes EvpEncryptor::encrypt(const bytes& plaintext, const bytes& key, const bytes& iv) const
{
    bytes ciphertext(plaintext.size() + static_cast<std::size_t>(EVP_CIPHER_block_size(mCipher)));
    int len, totalLen;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_EncryptInit_ex(ctx, mCipher, nullptr, key.data(), iv.data());
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), static_cast<int>(plaintext.size()));
    totalLen = len;

    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    totalLen += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(static_cast<size_t>(totalLen));

    return ciphertext;
}

bytes EvpEncryptor::decrypt(const bytes& ciphertext, const bytes& key, const bytes& iv) const
{
    bytes plaintext(ciphertext.size() + static_cast<std::size_t>(EVP_CIPHER_block_size(mCipher)), '\0');
    int len, totalLen;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    EVP_DecryptInit_ex(ctx, mCipher, nullptr, key.data(), iv.data());
    EVP_CIPHER_CTX_set_padding(ctx, 0);

    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len, reinterpret_cast<const unsigned char*>(ciphertext.data()), static_cast<int>(ciphertext.size()));
    totalLen = len;

    EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data() + len), &len);
    totalLen += len;

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(static_cast<size_t>(totalLen));

    return plaintext;
}

}
