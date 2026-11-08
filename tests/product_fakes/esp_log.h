#pragma once
#include <cstdio>
#define ESP_LOGI(tag, ...) do { (void)(tag); std::printf(__VA_ARGS__); std::puts(""); } while (0)
#define ESP_LOGW ESP_LOGI
