#pragma once
#include <atomic>
#include <cstdint>

namespace gagent::platform {
// Allocation failures may come from several tasks. The gate itself must never
// allocate, block or invoke the logger. A single budget covers all call sites.
class AllocationLog {
public:
    bool due(std::uint32_t now,std::uint32_t& suppressed) noexcept {
        const auto stamp=now|1u; // reserve zero for the first report; 2 ms resolution
        auto previous=last_.load(std::memory_order_relaxed);
        if((previous==0 || std::uint32_t(stamp-previous)>=10000u) &&
           last_.compare_exchange_strong(previous,stamp,std::memory_order_relaxed)){
            suppressed=suppressed_.exchange(0,std::memory_order_relaxed);
            return true;
        }
        suppressed_.fetch_add(1,std::memory_order_relaxed);
        return false;
    }
private:
    std::atomic<std::uint32_t> last_{0},suppressed_{0};
};
}
