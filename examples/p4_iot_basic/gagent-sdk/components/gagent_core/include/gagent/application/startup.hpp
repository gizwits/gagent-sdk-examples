#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/cloud/identity.hpp"
namespace gagent::runtime {
// Product-owned source fields bind a provider's persisted device credentials.
// Replacing these sources clears the listed identity fields in the same write.
struct IdentityScope {
    const std::string_view* sources;std::size_t source_count;
    const std::string_view* credentials;std::size_t credential_count;
};
// Explicit product input from a board/production source, never a customer fixture.
// These views must outlive Worker when passed through Worker::startup.
// No DID, passcode or Wi-Fi defaults: those belong to this device's transactions.
struct StartupDefaults;
using ProductTransition=bool(*)(const storage::Snapshot&,const StartupDefaults&,Change*,std::size_t&,std::size_t) noexcept;
struct StartupDefaults {
    std::string_view product_key,product_secret,attributes,host;
    std::uint16_t port=0;
    const Change* fields=nullptr;
    std::size_t field_count=0;
    const IdentityScope* identity=nullptr;
    ProductTransition transition=nullptr;
};
enum class StartupState {
    not_requested, pending, ready, needs_product, migration_required,
    storage_unavailable, invalid_product, product_mismatch, provider_mismatch, invalid_identity,
    entropy_failure, persistence_failure, handshake_failure, cancelled
};
struct StartupResult {
    StartupState state=StartupState::pending;
    BootState storage=BootState::unchecked;
    storage::WriteResult persistence{storage::Status::unchanged,storage::Applied::no,false};
};
// Call before device/radio/cloud activation on the single application Worker.
// Existing values win. Missing passcode may be generated only when no DID exists.
// V2 repair and confirmed total-loss cleanup are automatic; V1 migration is opt-in.
// Unrecognized formats/unparseable V2 are repaired or reset; I/O errors stop startup.
// Boot owns the snapshot buffers; do not call from the small ESP main-task stack.
StartupResult prepare_startup(Boot&,const StartupDefaults*,identity::Random,bool allow_migration=false) noexcept;
// Validated live MCU handshake: refresh supplied product fields atomically while
// retaining device credentials, Wi-Fi, regional endpoint and omitted extensions.
// Live startup accepts MCU PK/PS changes: a new PK replaces cloud identity atomically
// while retaining Wi-Fi. Cached fallback and live in-place refresh stay identity-bound.
StartupState validate_mcu_product(const storage::Snapshot&,const StartupDefaults&) noexcept;
StartupState stage_mcu_product(const storage::Snapshot&,const StartupDefaults&,storage::Snapshot&) noexcept;
StartupResult prepare_mcu_startup(Boot&,const StartupDefaults&,identity::Random) noexcept;
}
