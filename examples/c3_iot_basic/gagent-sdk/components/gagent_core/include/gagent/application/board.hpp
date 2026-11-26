#pragma once
#include <cstdint>
namespace gagent::runtime {class Boot;}
namespace gagent::board {
enum class Mode { soc, mcu };
struct Profile {
    Mode mode = Mode::soc;
    bool beacon = false;
    bool mcu_ota = false;
    std::uint32_t baud = 115200;
    std::uint32_t heartbeat_ms = 55000;
    std::uint32_t ack_timeout_ms = 600;
    bool storage_writable = false;
    bool module_ota = true;
};
constexpr bool valid(const Profile& p) noexcept {
    return (p.mode==Mode::soc || p.mode==Mode::mcu) &&
        (!p.mcu_ota || p.mode==Mode::mcu) && p.baud>=1200 && p.baud<=4000000 &&
        (p.mode!=Mode::mcu || p.baud==9600 || p.baud==19200 || p.baud==115200 || p.baud==460800) &&
        p.heartbeat_ms<=3600000 && p.ack_timeout_ms<=10000;
}
// Board-owned product logic; validation profile links the complete application but keeps it disabled.
// Called once after storage inspection, including empty/unsupported/error states.
// The selected profile controls write access; inspection itself never writes.
// Inspect Boot before starting services; only explicit production seeds may initialize erased flash.
// It must not retain borrowed stack views.
void start(runtime::Boot&) noexcept;
}
