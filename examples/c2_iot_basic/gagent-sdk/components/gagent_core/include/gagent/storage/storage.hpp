#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <memory>

namespace gagent::runtime { class Boot; }

namespace gagent::storage {

constexpr std::size_t sector_size = 4096;
constexpr std::size_t legacy_header_size = 264;
constexpr std::size_t max_body_size = sector_size - legacy_header_size;
using Record = std::array<std::uint8_t, sector_size>;
using RecordPair = std::array<Record, 2>;
using Md5 = bool (*)(const std::uint8_t*, std::size_t, std::uint8_t*);

enum class Status {
    ok, empty, corrupt, unsupported_format, invalid_fields, too_large,
    io_error, hash_error, invalid_layout, needs_initialization,
    protected_data, degraded, unchanged, no_memory
};
enum class Slot : std::uint8_t { primary, backup };
enum class Applied { no, yes, unknown };

struct Snapshot {
    std::array<std::uint8_t, max_body_size> body{};
    std::size_t size = 0;
    std::string_view text() const noexcept;
};

// Validate known field sizes; unknown fields remain byte-for-byte in the snapshot.
// Rules and their key strings must outlive their use. Maxima exclude any trailing NUL.
struct FieldRule {
    std::string_view key;
    std::size_t max_value_size;
    bool required = false;
};
struct Schema {
    const FieldRule* fields = nullptr;
    std::size_t count = 0;
};

struct FieldValue {std::string_view key,value;};

class Codec {
public:
    explicit Codec(Md5 md5) noexcept : md5_(md5) {}
    Status validate(const Snapshot&, Schema = {}) const noexcept;
    Status validate(std::string_view, Schema = {}) const noexcept;
    Status decode(const Record&, Snapshot&, Schema = {}) const noexcept;
    Status encode(const Snapshot&, Record&, Schema = {}) const noexcept;
    // Set replaces one exact key in place/order, removes it for an empty value,
    // and preserves every unrelated key, including unknown keys. Atomic in RAM.
    Status set(const Snapshot&, std::string_view key, std::string_view value,
               Snapshot& output, Schema = {}) const noexcept;
    // Merge a batch into caller-owned scratch, checking only the final snapshot.
    // Preserve existing key order/unknown bytes; append new keys in change order.
    // Inputs must not alias output (views into source are allowed). Source/output
    // alias is rejected untouched; otherwise output is cleared on error. No heap/snapshot scratch.
    Status merge(const Snapshot&,const FieldValue*,std::size_t,Snapshot& output,Schema = {}) const noexcept;
    // Build in caller-owned scratch; inputs must not alias output. Output is cleared on error.
    // Empty values are omitted; repeated keys and malformed fields are rejected centrally.
    Status build(const FieldValue*,std::size_t,Snapshot& output,Schema = {}) const noexcept;
    static std::string_view get(const Snapshot&, std::string_view key) noexcept;
private:
    Md5 md5_;
};

// Adapter contract: two disjoint sectors; operations never cross a slot boundary.
// A successful read/write means ALL requested bytes. Errors may have partial effects.
// No retries or recovery in the adapter; errors are returned to the store.
class FlashPair {
public:
    virtual ~FlashPair() = default;
    virtual bool valid_layout() const noexcept = 0;
    virtual bool read(Slot, std::size_t offset, std::uint8_t*, std::size_t) noexcept = 0;
    virtual bool erase(Slot) noexcept = 0;
    virtual bool write(Slot, std::size_t offset, const std::uint8_t*, std::size_t) noexcept = 0;
};

struct LoadResult {
    Status status = Status::empty;
    Status primary = Status::empty;
    Status backup = Status::empty;
    Slot selected = Slot::primary;
    bool has_snapshot = false;
    bool redundant = false;
    bool writable = false;
};
struct WriteResult {
    Status status;
    Applied applied;
    bool redundant;
};

// Single owner: the caller must serialize load/repair/commit/initialize.
// No implicit erases on reads, factory reset, identity creation or registration.
// Owns ~8 KiB inline scratch plus a fallible 8 KiB record pair; not for task stacks.
class ConfigStore {
public:
    ConfigStore(FlashPair& flash, Md5 md5, Schema schema = {}) noexcept;
    ConfigStore(FlashPair&, Md5, Schema, std::unique_ptr<RecordPair>) noexcept;
    ConfigStore(const ConfigStore&) = delete;
    ConfigStore& operator=(const ConfigStore&) = delete;
    LoadResult load(Snapshot& output) noexcept;
    WriteResult repair() noexcept;
    WriteResult commit(const Snapshot&) noexcept;
    // Explicit factory initialization only; both sectors must be entirely erased.
    WriteResult initialize(const Snapshot&) noexcept;
private:
    friend class gagent::runtime::Boot;
    LoadResult inspect() noexcept;
    Status replace(Slot, const Record&) noexcept;
    WriteResult finish(Status cause, const Record& desired) noexcept;
    bool matches(Slot, const Record&) const noexcept;
    FlashPair& flash_;
    Codec codec_;
    Schema schema_;
    std::unique_ptr<RecordPair> records_;
    Record candidate_{};
    Snapshot scratch_{};
};

} // namespace gagent::storage
