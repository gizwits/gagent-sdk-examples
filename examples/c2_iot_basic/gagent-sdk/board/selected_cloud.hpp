#pragma once
// Generated composition only. Neither core nor ESP drivers include this header.
#include "gagent/cloud/gizwits.hpp"
#include "gagent/esp_entropy.hpp"
#include "mbedtls_md5.hpp"
#include "mbedtls_aes.hpp"
namespace gagent::firmware {
using CloudOptions=cloud::GizwitsOptions;
class SelectedCloud : public cloud::Gizwits {
public:
    explicit SelectedCloud(const CloudOptions& options={}) noexcept
        : cloud::Gizwits(options,{platform::md5,platform::encrypt_legacy_ecb,platform::decrypt_legacy_ecb_raw,platform::random_bytes}) {}
};
}
