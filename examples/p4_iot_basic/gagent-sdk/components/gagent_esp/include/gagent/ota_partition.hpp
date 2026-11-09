#pragma once
#include "gagent/ota/ota_cache.hpp"
#include "esp_partition.h"
namespace gagent::platform {
// Borrows an IDF descriptor; never falls back to an application/config partition.
class OtaPartition final : public ota::CacheStore {
public:
    OtaPartition(const esp_partition_t* partition,const board::Profile& profile) noexcept : partition_(partition),enabled_(board::valid(profile) && profile.mode==board::Mode::mcu && profile.mcu_ota) {}
    std::size_t capacity() const noexcept override;
    bool erase(std::size_t offset,std::size_t length) noexcept override;
    bool write(std::size_t offset,protocol::Bytes) noexcept override;
    bool read(std::size_t offset,std::uint8_t*,std::size_t) noexcept override;
private:
    bool valid() const noexcept;
    bool range(std::size_t offset,std::size_t length) const noexcept;
    const esp_partition_t* partition_;
    bool enabled_;
};
}
