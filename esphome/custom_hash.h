#include "mbedtls/sha256.h"
#include <string>
#include <cstdio>

std::string get_sha256(const std::string& input) {
    unsigned char hash[32];
    mbedtls_sha256_context ctx;

    mbedtls_sha256_init(&ctx);

    // 0 indicates SHA-256, not SHA-224.
    // Updated for mbedTLS 3.x API (removed _ret suffix)
    mbedtls_sha256_starts(&ctx, 0);
    mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
    mbedtls_sha256_finish(&ctx, hash);
    mbedtls_sha256_free(&ctx);

    char hexString[65];
    for(int i = 0; i < 32; i++) {
        sprintf(&hexString[i*2], "%02x", hash[i]);
    }

    return std::string(hexString);
}
