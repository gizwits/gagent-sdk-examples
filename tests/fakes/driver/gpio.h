#pragma once
#include <cstdint>
using esp_err_t=int;
constexpr int ESP_OK=0, ESP_FAIL=-1;
enum gpio_num_t {GPIO_NUM_8=8,GPIO_NUM_9=9,GPIO_NUM_35=35,GPIO_NUM_51=51};
constexpr int GPIO_MODE_INPUT=1,GPIO_PULLUP_ENABLE=1,GPIO_PULLDOWN_DISABLE=0,GPIO_INTR_DISABLE=0;
struct gpio_config_t {std::uint64_t pin_bit_mask;int mode,pull_up_en,pull_down_en,intr_type;};
esp_err_t gpio_config(const gpio_config_t*);
int gpio_get_level(gpio_num_t);
const char* esp_err_to_name(esp_err_t);
