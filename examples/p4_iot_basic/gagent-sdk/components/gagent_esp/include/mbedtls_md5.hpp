#pragma once
#include <cstddef>
#include <cstdint>

namespace gagent::platform {
// Same implementation used by host tests and the future ESP-IDF composition root.
bool sha256(const std::uint8_t* input, std::size_t size, std::uint8_t* digest) noexcept;
bool md5(const std::uint8_t* input, std::size_t size, std::uint8_t* digest) noexcept;
}
