#pragma once
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_log.h"
#if defined(GAGENT_COMPACT_2M) && CONFIG_LOG_MAXIMUM_LEVEL < 3
#include "esp_rom_sys.h"
#endif
#endif
namespace gagent::platform {
#ifdef GAGENT_HEAP_LAYOUT_DIAGNOSTICS
void log_heap_layout(const char* phase) noexcept;
#endif
// Diagnostic snapshots only: no allocations, extra task, or timer callback.
inline void log_memory(const char* phase) noexcept {
// A disabled console may mean UART0 is now a speech protocol port. ROM
// printing bypasses ESP_LOG level filtering and must be disabled as well.
#if defined(ESP_PLATFORM) && !defined(CONFIG_ESP_CONSOLE_NONE)
    constexpr auto internal_dma=MALLOC_CAP_INTERNAL|MALLOC_CAP_DMA;
#if defined(GAGENT_COMPACT_2M) && CONFIG_LOG_MAXIMUM_LEVEL < 3
    // Keep the existing snapshots observable while all other compact INFO
    // logging remains excluded; no diagnostic buffers or extra task are added.
    esp_rom_printf("[gagent_heap] %s bytes: free=%u min=%u largest=%u internal_dma_free=%u internal_dma_largest=%u\n",
#else
    ESP_LOGI("gagent_heap", "%s bytes: free=%u min=%u largest=%u internal_dma_free=%u internal_dma_largest=%u",
#endif
        phase,
        unsigned(heap_caps_get_free_size(MALLOC_CAP_8BIT)),
        unsigned(heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT)),
        unsigned(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT)),
        unsigned(heap_caps_get_free_size(internal_dma)),
        unsigned(heap_caps_get_largest_free_block(internal_dma)));
#ifdef GAGENT_HEAP_LAYOUT_DIAGNOSTICS
    log_heap_layout(phase);
#endif
#else
    (void)phase;
#endif
}
}
