#pragma once
namespace gagent::build {
#ifdef GAGENT_COMPACT_2M
inline constexpr bool lan = false;
inline constexpr bool mdns = false;
#else
inline constexpr bool lan = true;
inline constexpr bool mdns = true;
#endif
#if defined(GAGENT_COMPACT_2M) || defined(GAGENT_BLE_ONLY_PROVISIONING)
inline constexpr bool smartconfig = false;
#else
inline constexpr bool smartconfig = true;
#endif
#ifdef GAGENT_BLE_ONLY_PROVISIONING
inline constexpr bool softap = false;
#else
inline constexpr bool softap = true;
#endif
}
