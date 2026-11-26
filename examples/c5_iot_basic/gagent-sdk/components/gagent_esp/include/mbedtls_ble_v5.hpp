#pragma once
#include "gagent/network/ble_v5.hpp"
namespace gagent::platform {
ble::v5::Crypto ble_v5_crypto(identity::Random) noexcept;
}
