#pragma once

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#endif

namespace gagent::board::diagnostics {
// 只采样堆统计，不申请缓冲区或创建任务；单位为字节。
inline void log_memory(const char* phase) noexcept {
#ifdef ESP_PLATFORM
    const auto log_heap = [phase](const char* name, uint32_t caps) {
        multi_heap_info_t info{};
        heap_caps_get_info(&info, caps);
        ESP_LOGI("p4-memory", "%s %s: used=%zu free=%zu min_free=%zu largest=%zu bytes",
                 phase, name, info.total_allocated_bytes, info.total_free_bytes,
                 info.minimum_free_bytes, info.largest_free_block);
    };
    // 分开统计，避免 PSRAM 的剩余容量掩盖内部 RAM 紧张。
    log_heap("internal", MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    log_heap("psram", MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    (void)phase;
#endif
}
}
