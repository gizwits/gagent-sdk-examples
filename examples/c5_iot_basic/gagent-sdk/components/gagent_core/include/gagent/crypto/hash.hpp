#pragma once
#include <psa/crypto.h>
#include <cstddef>
#include <cstdint>

namespace gagent::crypto {
// Volatile operations only: OTA verification never creates persistent PSA keys.
struct HashContext {
    psa_hash_operation_t operation = PSA_HASH_OPERATION_INIT;
    HashContext() = default;
    HashContext(const HashContext&) = delete;
    HashContext& operator=(const HashContext&) = delete;
    ~HashContext() { psa_hash_abort(&operation); }
};
using md5_context = HashContext;
using sha256_context = HashContext;
inline void md5_init(HashContext*) {}
inline void sha256_init(HashContext*) {}
inline void md5_free(HashContext* context) { psa_hash_abort(&context->operation); }
inline void sha256_free(HashContext* context) { psa_hash_abort(&context->operation); }
inline int hash_start(HashContext* context, psa_algorithm_t algorithm) {
    psa_hash_abort(&context->operation);
    const auto status = psa_crypto_init();
    return status == PSA_SUCCESS ? psa_hash_setup(&context->operation, algorithm) : status;
}
inline int md5_starts(HashContext* context) { return hash_start(context, PSA_ALG_MD5); }
inline int sha256_starts(HashContext* context, int is224) {
    return hash_start(context, is224 ? PSA_ALG_SHA_224 : PSA_ALG_SHA_256);
}
inline int md5_update(HashContext* context, const unsigned char* bytes, std::size_t size) {
    return psa_hash_update(&context->operation, bytes, size);
}
inline int sha256_update(HashContext* context, const unsigned char* bytes, std::size_t size) {
    return psa_hash_update(&context->operation, bytes, size);
}
inline int hash_finish(HashContext* context, unsigned char* output, std::size_t capacity) {
    std::size_t written = 0;
    return psa_hash_finish(&context->operation, output, capacity, &written);
}
inline int md5_finish(HashContext* context, unsigned char* output) { return hash_finish(context, output, 16); }
inline int sha256_finish(HashContext* context, unsigned char* output) { return hash_finish(context, output, 32); }
inline int hash(psa_algorithm_t algorithm, const unsigned char* input, std::size_t size,
                unsigned char* output, std::size_t capacity) {
    std::size_t written = 0;
    const auto status = psa_crypto_init();
    return status == PSA_SUCCESS ? psa_hash_compute(algorithm, input, size, output, capacity, &written) : status;
}
inline int md5(const unsigned char* input, std::size_t size, unsigned char* output) {
    return hash(PSA_ALG_MD5, input, size, output, 16);
}
inline int sha256(const unsigned char* input, std::size_t size, unsigned char* output, int is224) {
    return hash(is224 ? PSA_ALG_SHA_224 : PSA_ALG_SHA_256, input, size, output, 32);
}
}
