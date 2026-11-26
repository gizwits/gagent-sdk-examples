#pragma once
#include "product_secrets.hpp"
#include "generated/product_identity.hpp"
namespace gagent::board::configuration {
template <unsigned N> constexpr bool credential_valid(const char (&value)[N]) {
    if (N != 33) return false;
    bool nonzero = false;
    for (unsigned i = 0; i + 1 < N; ++i) {
        const char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
        nonzero = nonzero || c != '0';
    }
    return nonzero;
}
template <unsigned N, unsigned M> constexpr bool same_key(const char (&a)[N], const char (&b)[M]) {
    if (N != M) return false;
    for (unsigned i = 0; i < N; ++i) if (a[i] != b[i]) return false;
    return true;
}
static_assert(credential_valid(GAGENT_SOC_PRODUCT_KEY), "Fill a 32-character hexadecimal PK in product_secrets.hpp");
static_assert(credential_valid(GAGENT_SOC_PRODUCT_SECRET), "Fill a 32-character hexadecimal PS in product_secrets.hpp");
static_assert(same_key(GAGENT_SOC_PRODUCT_KEY, generated_product_key),
              "PK changed: run python3 gagent-sdk/configure_product.py again before building");
inline constexpr char product_secret[] = GAGENT_SOC_PRODUCT_SECRET;
inline constexpr char cloud_host[] = "api.gizwits.com";
inline constexpr unsigned short cloud_port = 80;
inline constexpr char bluetooth_name[] = "GAgent-C5";
inline constexpr char attributes[] = "0000000000000002";
}
