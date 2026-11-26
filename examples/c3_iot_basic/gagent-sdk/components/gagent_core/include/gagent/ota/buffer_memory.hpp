#pragma once
#include <cstddef>
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

namespace gagent::ota {
// Keep headroom for concurrent network/control allocations and a contiguous block.
inline bool buffer_fits(std::size_t bytes,std::size_t free,std::size_t largest,
                        std::size_t reserve=16384) noexcept {
    return free>=reserve && bytes<=free-reserve && largest>=8192 && bytes<=largest-8192;
}
inline bool buffer_memory_available(std::size_t bytes,std::size_t reserve=16384) noexcept {
#ifdef ESP_PLATFORM
    constexpr auto caps=MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT;
    return buffer_fits(bytes,heap_caps_get_free_size(caps),heap_caps_get_largest_free_block(caps),reserve);
#else
    (void)bytes;(void)reserve;
    return true;
#endif
}
}
