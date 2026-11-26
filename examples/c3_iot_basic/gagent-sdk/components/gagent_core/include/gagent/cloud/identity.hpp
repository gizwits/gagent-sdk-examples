#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace gagent::identity {
using Random=bool(*)(std::uint8_t*,std::size_t) noexcept;
using Passcode=std::array<char,11>;
// Legacy wire alphabet A..Y and ten characters. Bounded rejection sampling,
// shared by first boot, reset recovery and every onboarding transport.
bool generate_passcode(Random,Passcode&) noexcept;
}
