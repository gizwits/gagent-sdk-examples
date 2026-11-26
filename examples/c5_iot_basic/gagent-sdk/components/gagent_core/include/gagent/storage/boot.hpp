#pragma once
#include "gagent/storage/migration.hpp"
namespace gagent::runtime {
enum class BootState { unchecked, empty, migration_required, recovery_required, ready, unsupported, damaged, io_failure, invalid_layout };
using Change=storage::FieldValue;
// Sole runtime configuration owner. All methods run on the application worker.
// Runtime V1 migration and V2 operations reuse snapshots and stream sector I/O
// without heap scratch.
// Only the current and staged snapshots remain resident; never put Boot on a task stack.
class Boot {
public:
    Boot(storage::LegacyPair& flash, storage::Md5 md5,config::ResetTransform reset=nullptr) noexcept
        : flash_(flash),md5_(md5),reset_(reset) {}
    Boot(const Boot&)=delete;
    Boot& operator=(const Boot&)=delete;
    BootState inspect() noexcept;
    // A new startup must validate Flash afresh, never reuse a prior runtime view.
    BootState inspect_startup() noexcept {snapshot_valid_=false;return inspect();}
    storage::WriteResult migrate() noexcept;
    // Startup recovery only: discard unusable/unknown data when neither copy is valid.
    // Preserves recognized V1 and I/O/hash/layout failures. Success is empty.
    storage::Status discard_damaged() noexcept;
    storage::WriteResult reset(config::Reset) noexcept;
    void reset_transform(config::ResetTransform transform) noexcept {reset_=transform;}
    // Batch validation is atomic in RAM; Flash commit uses the established redundant store.
    // Requires a ready V2 configuration. No implicit migration, repair or factory defaults.
    storage::WriteResult apply(const Change*,std::size_t count) noexcept;
    // Only OTA counters may use best-effort persistence. After storage degrades,
    // stop counter I/O until explicit successful inspection/recovery or reboot.
    storage::WriteResult apply_ota_counter(const Change&) noexcept;
    // Explicit production/board seed only, allowed only if both slots are wholly erased.
    storage::WriteResult initialize(const storage::Snapshot& seed) noexcept;
    // Build an explicit seed in owned scratch memory; avoids a caller-owned 4KB stack snapshot.
    storage::WriteResult initialize(const Change*,std::size_t count) noexcept;
    // Physical storage state; failure does not invalidate a previously verified
    // runtime snapshot. Critical writes still require a ready physical store.
    BootState state() const noexcept { return state_; }
    // Publish only complete, verified configurations. Failed writes/reads retain
    // the previous runtime snapshot; a cold owner never invents a fallback.
    const storage::Snapshot* snapshot() const noexcept { return snapshot_valid_ ? &snapshot_ : nullptr; }
private:
    storage::WriteResult finish(storage::WriteResult) noexcept;
    storage::LegacyPair& flash_;
    storage::Md5 md5_;
    storage::Snapshot snapshot_{},candidate_{};
    BootState state_=BootState::unchecked;
    bool snapshot_valid_=false,counter_persistence_suspended_=false;
    config::ResetTransform reset_=nullptr;
};
}
