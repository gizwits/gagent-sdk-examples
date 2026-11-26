#pragma once
#include "gagent/diagnostics/options.hpp"
// Use the SDK's compile-time and per-tag runtime filters; never bypass them.
// Host builds have no logging dependency or argument evaluation.
#ifdef ESP_PLATFORM
#include "esp_log.h"
#define GAGENT_LOGE(...) ESP_LOGE(__VA_ARGS__)
#define GAGENT_LOGW(...) ESP_LOGW(__VA_ARGS__)
#define GAGENT_LOGI(...) ESP_LOGI(__VA_ARGS__)
#define GAGENT_LOGD(...) ESP_LOGD(__VA_ARGS__)
#else
#define GAGENT_LOGE(...) ((void)0)
#define GAGENT_LOGW(...) ((void)0)
#define GAGENT_LOGI(...) ((void)0)
#define GAGENT_LOGD(...) ((void)0)
#endif

// Board-selected verbosity for successful control requests only.
#if GAGENT_CONTROL_LOG_DEBUG
#define GAGENT_LOG_CONTROL(...) GAGENT_LOGD(__VA_ARGS__)
#else
#define GAGENT_LOG_CONTROL(...) GAGENT_LOGI(__VA_ARGS__)
#endif
