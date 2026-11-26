#pragma once
#include "gagent/ota/mcu_ota.hpp"
#include <string_view>
namespace gagent::ota {
// Pure bounded decoding, shared by transports; no storage, task or application effects.
class ImageHeaders {
public:
    void add(std::string_view name,std::string_view value) noexcept;
    // Null when offer() would accept the image. Otherwise a stable reason: invalid, md5, length, empty, capacity, mismatch.
    const char* rejection(std::int64_t received_length,std::uint32_t capacity) const noexcept;
    std::uint32_t size() const noexcept {return value_.size;}
    bool offer(std::int64_t received_length,std::uint32_t capacity,Offer&) const noexcept;
private:
    Offer value_{};
    bool md5_=false,length_=false,disposition_=false,invalid_=false;
};
}
