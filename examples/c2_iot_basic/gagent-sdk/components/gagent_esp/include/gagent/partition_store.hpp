#pragma once
#include "gagent/storage/migration.hpp"
#include "esp_partition.h"

namespace gagent::platform {
enum class Access { read_only, read_write };

// Borrows stable IDF partition descriptors. Only the FIRST 4 KiB of each
// partition is exposed, even when legacy partitions are larger.
class PartitionStore final : public storage::LegacyPair {
public:
    PartitionStore(const esp_partition_t* primary, const esp_partition_t* backup,
                   Access access = Access::read_only) noexcept;
    bool valid_layout() const noexcept override;
    std::size_t sector_count(storage::Slot) const noexcept override;
    bool read_second(storage::Slot, storage::Record&) noexcept override;
    bool read_second_range(storage::Slot,std::size_t,std::uint8_t*,std::size_t) noexcept override;
    bool read(storage::Slot, std::size_t, std::uint8_t*, std::size_t) noexcept override;
    bool erase(storage::Slot) noexcept override;
    bool write(storage::Slot, std::size_t, const std::uint8_t*, std::size_t) noexcept override;
    esp_err_t last_error() const noexcept { return error_; }
private:
    const esp_partition_t* checked(storage::Slot, std::size_t, std::size_t) noexcept;
    bool can_write(const esp_partition_t*) noexcept;
    const esp_partition_t* partitions_[2];
    Access access_;
    esp_err_t error_ = ESP_OK;
};
}
