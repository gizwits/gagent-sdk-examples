#pragma once
#ifndef GAGENT_BLE_PROTOCOL_VERSION
#define GAGENT_BLE_PROTOCOL_VERSION 2
#endif
#if GAGENT_BLE_PROTOCOL_VERSION != 2 && GAGENT_BLE_PROTOCOL_VERSION != 5
#error "Unsupported BLE application protocol"
#endif
namespace gagent::ble {
inline constexpr unsigned protocol_version=GAGENT_BLE_PROTOCOL_VERSION;
}
#ifndef GAGENT_BLE_V5_OTA
#define GAGENT_BLE_V5_OTA 1
#endif
#ifndef GAGENT_BLE_V5_SECURITY
#define GAGENT_BLE_V5_SECURITY 1
#endif
