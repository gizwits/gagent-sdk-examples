#pragma once
#include "gagent/storage/config.hpp"
#include <new>
namespace gagent::storage {
enum class Format { unknown, v1, v2 };
struct LegacyResult { Status status; Format format; bool checksummed; };
// V1 second sector is supplied only after checking actual partition capacity.
// V0/OLD and other unknown formats are never decoded or overwritten.
struct LegacyWorkspace { Snapshot candidate; };
LegacyResult decode_legacy(const Record&, const Record* second, Snapshot&, Md5, LegacyWorkspace&) noexcept;
class LegacyPair : public FlashPair {
public:
    virtual std::size_t sector_count(Slot) const noexcept = 0;
    virtual bool read_second(Slot, Record&) noexcept = 0;
    // Optional bounded V1 access for runtime migration. A missing implementation
    // fails closed; never fall back to a sector-sized allocation or stack buffer.
    virtual bool read_second_range(Slot,std::size_t,std::uint8_t*,std::size_t) noexcept {return false;}
};
struct MigrationResult { Status status; Format source; bool has_snapshot; bool checksummed; Status detail=Status::ok; bool redundant=false; bool migration_required=false; };
// Owns bounded scratch buffers. Long-lived, serialized owner; no automatic startup writes.
class Migration {
public:
    Migration(LegacyPair& flash, Md5 md5) noexcept
        : Migration(flash,md5,std::unique_ptr<RecordPair>(new(std::nothrow) RecordPair{})) {}
    Migration(LegacyPair& flash, Md5 md5, std::unique_ptr<RecordPair> records) noexcept
        : flash_(flash), md5_(md5), codec_(md5), records_(std::move(records)) {}
    MigrationResult load(Snapshot&) noexcept;
    WriteResult migrate() noexcept;
private:
    friend class gagent::runtime::Boot;
    Status replace(Slot) noexcept;
    LegacyPair& flash_;
    Md5 md5_;
    Codec codec_;
    // Load: current first sector + V1 second sector (or retained V2 primary).
    // Write: encoded destination + readback. No whole-sector buffers on the stack.
    std::unique_ptr<RecordPair> records_;
    Snapshot desired_{};
    LegacyWorkspace workspace_{};
    LegacyResult result_[2]{};
    unsigned selected_ = 0;
};
}
