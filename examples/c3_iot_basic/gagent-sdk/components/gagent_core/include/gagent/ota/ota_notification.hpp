#pragma once
#include "gagent/ota/ota_check.hpp"
namespace gagent::ota {
// Notification types are separate from module/MCU image destinations:
// voice is handed to the peripheral OTA callback after a combined check.
enum class NotificationTarget {module=1,mcu=2,voice=7};
struct Notification {NotificationTarget target=NotificationTarget::module;protocol::Bytes id;};
// Server notice 0x0211. ID is opaque (0..22 bytes), not the device's DID.
// Subdevice types 3..5 belong to the deferred gateway feature.
protocol::Status notification(const protocol::Frame&,Notification&) noexcept;
}
