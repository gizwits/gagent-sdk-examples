#pragma once
#include <cstddef>
#include <cstdint>
namespace gagent::platform {
// Exactly one attempt per boot, before RF/ADC/I2S initialization. The composition root owns
// that ordering. Once seeded, later calls never touch the hardware entropy source again.
bool prepare_entropy() noexcept;
// Identity::Random contract (at most 32 bytes). Uses the seeded official CTR-DRBG even
// while the radios are off; exhaustion/error fails closed until a controlled reboot.
bool random_bytes(std::uint8_t*,std::size_t) noexcept;
}
