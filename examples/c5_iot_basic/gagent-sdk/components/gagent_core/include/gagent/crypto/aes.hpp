#pragma once
#include <psa/crypto.h>
#include <cstddef>
#include <cstdint>

namespace gagent::crypto {
inline constexpr int aes_encrypt = 1, aes_decrypt = 0;
struct aes_context {
    psa_key_id_t key = 0;
    aes_context() = default;
    aes_context(const aes_context&) = delete;
    aes_context& operator=(const aes_context&) = delete;
    ~aes_context() { if (key) psa_destroy_key(key); }
};
inline void aes_init(aes_context*) {}
inline void aes_free(aes_context* context) {
    if (context->key) psa_destroy_key(context->key);
    context->key = 0;
}
inline int aes_setkey(aes_context* context, const unsigned char* key, unsigned bits) {
    aes_free(context);
    auto status = psa_crypto_init();
    if (status != PSA_SUCCESS) return status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, bits);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
    status = psa_import_key(&attributes, key, bits / 8, &context->key);
    psa_reset_key_attributes(&attributes);
    return status;
}
inline int aes_setkey_enc(aes_context* context, const unsigned char* key, unsigned bits) { return aes_setkey(context, key, bits); }
inline int aes_setkey_dec(aes_context* context, const unsigned char* key, unsigned bits) { return aes_setkey(context, key, bits); }
inline int aes_crypt_ecb(aes_context* context, int mode, const unsigned char* input, unsigned char* output) {
    std::size_t written = 0;
    const auto status = mode == aes_encrypt
        ? psa_cipher_encrypt(context->key, PSA_ALG_ECB_NO_PADDING, input, 16, output, 16, &written)
        : psa_cipher_decrypt(context->key, PSA_ALG_ECB_NO_PADDING, input, 16, output, 16, &written);
    return status == PSA_SUCCESS && written != 16 ? PSA_ERROR_GENERIC_ERROR : status;
}
}
