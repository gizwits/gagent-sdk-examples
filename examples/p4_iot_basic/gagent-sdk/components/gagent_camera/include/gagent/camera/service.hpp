#pragma once
#include "gagent/camera/control.hpp"
namespace gagent::camera {
// Process-lifetime service; created only by camera-enabled MCU startup glue.
Control& service() noexcept;
}
