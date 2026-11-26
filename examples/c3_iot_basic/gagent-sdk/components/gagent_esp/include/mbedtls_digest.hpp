#pragma once
#include "gagent/ota/ota_verify.hpp"
#include "gagent/crypto/hash.hpp"
namespace gagent::platform {
class Md5Digest final : public ota::Digest {
public:
    Md5Digest() noexcept {gagent::crypto::md5_init(&context_);}
    ~Md5Digest() {gagent::crypto::md5_free(&context_);}
    Md5Digest(const Md5Digest&)=delete;
    Md5Digest& operator=(const Md5Digest&)=delete;
    bool begin() noexcept override;
    bool update(protocol::Bytes) noexcept override;
    bool finish(std::array<std::uint8_t,16>&) noexcept override;
private:
    gagent::crypto::md5_context context_{};
    bool active_=false;
};
}
