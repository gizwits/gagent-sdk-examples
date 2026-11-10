#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { int pin; bool active_high; uint32_t delay_ms; } gagent_p4_power_config_t;
// Customer-generated binding from board.json.
gagent_p4_power_config_t gagent_p4_board_power(void);
esp_err_t gagent_p4_mac(uint8_t mac[6]);
// pin=-2 selects customer board defaults. Configure before connecting. Once connected, only identical settings are accepted.
esp_err_t gagent_p4_configure_power(int pin, bool active_high, uint32_t delay_ms);
// Serialized BLE manager lifecycle; stop only after the NimBLE host exits.
esp_err_t gagent_p4_ble_start(void);
esp_err_t gagent_p4_ble_stop(void);
#ifdef __cplusplus
}
#endif
