#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace gagent::time {
class Source {
public:
    virtual ~Source() = default;
    // Nonzero revision changes when time/timezone synchronization is accepted, not as seconds tick.
    virtual std::uint64_t revision() const noexcept {return 0;}
    virtual bool unix_time(std::uint64_t now, std::uint64_t& seconds) const noexcept = 0;
};
// Strict decimal seconds east of UTC. Absent configuration means UTC.
bool timezone(std::string_view, std::int32_t& seconds) noexcept;
// Legacy MCU calendar (7 bytes) followed by the original BE32 Unix timestamp.
// Failure leaves output unchanged; no wrapping into the 32-bit wire format.
bool encode(std::uint64_t seconds, std::int32_t timezone, std::array<std::uint8_t,11>&) noexcept;
}
