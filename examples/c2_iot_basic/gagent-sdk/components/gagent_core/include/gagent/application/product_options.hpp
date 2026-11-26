#pragma once
#include "gagent/protocol/management.hpp"
#include "gagent/cloud/cloud.hpp"
#include "gagent/network/local.hpp"
namespace gagent::runtime {
// Board defaults and the resolved run-time product policy. All saved strings are copied;
// only cloud protocol and module build strings remain views of long-lived board constants.
struct ProductOptions {
    board::Profile profile{};
    local::Discovery discovery{};
    management::Module module{};
    cloud::Metadata cloud{};
    wifi::AccessPoint softap{};
    bool skip_ble_login=false;
    bool beacon_on_start=false;
    bool dual_channel=true; // runMode: 0/2 dual, 1 single; omitted uses the default (dual).
    onboarding::Mode configuration_mode=onboarding::Mode::softap;
};
enum class ProductError {none,storage,board,identity,attributes,version,serial_timing,timezone,softap,bluetooth,configuration_mode,onboarding};
// No Flash writes or I/O. Saved product fields override explicit board defaults.
// Failed resolution leaves output unchanged, so no module can consume a partial policy.
ProductError resolve_product(const storage::Snapshot&,const ProductOptions& defaults,ProductOptions& output) noexcept;
}
