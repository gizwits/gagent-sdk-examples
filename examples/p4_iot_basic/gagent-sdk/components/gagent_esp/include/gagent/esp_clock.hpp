#pragma once
#include "gagent/cloud/time.hpp"

namespace gagent::platform {
// Bridge the existing synchronized cloud clock to libc; no NTP task or socket.
class EspClock {
public:
    using Apply=bool (*)(std::uint64_t) noexcept;
    explicit EspClock(Apply apply=apply_utc) noexcept:apply_(apply) {}
    bool update(const time::Source&,std::uint64_t now) noexcept;
private:
    static bool apply_utc(std::uint64_t) noexcept;
    Apply apply_;
    std::uint64_t revision_=0;
};
}
