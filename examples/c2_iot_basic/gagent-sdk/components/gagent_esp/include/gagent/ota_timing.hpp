#pragma once
#include "gagent/diagnostics/options.hpp"
#include <cstdint>

#if defined(ESP_PLATFORM) && CONFIG_LOG_MAXIMUM_LEVEL >= 3 && !defined(CONFIG_ESP_CONSOLE_NONE)
#define GAGENT_OTA_TIMING 1
#include "esp_timer.h"

namespace gagent::platform {
// Report the same block boundaries as DownloadController, including a short final block.
struct OtaTimingWindow {
    unsigned chunks=0;
    std::uint64_t next_byte=0;

    unsigned advance(std::uint32_t bytes,std::uint32_t total) noexcept {
        ++chunks;
        if(bytes<next_byte)return 0;
        const auto tenth=std::uint64_t(bytes)*10/total;
        next_byte=((tenth+1)*total+9)/10;
        return unsigned(tenth)*10;
    }
};
}
#else
#define GAGENT_OTA_TIMING 0
#endif
