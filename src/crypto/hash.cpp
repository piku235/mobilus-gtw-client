#include "hash.h"

#include <openssl/evp.h>
#include <openssl/sha.h>

namespace jungi::mobilus_gtw_client::crypto {

bytes sha256(const std::string& text)
{
    bytes hash(SHA256_DIGEST_LENGTH);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();

    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, text.data(), text.length());
    EVP_DigestFinal_ex(ctx, hash.data(), nullptr);

    EVP_MD_CTX_free(ctx);

    return hash;
}

}
