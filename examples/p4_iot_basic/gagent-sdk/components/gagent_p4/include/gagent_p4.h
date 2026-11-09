#pragma once
#include "esp_err.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t gagent_p4_mac(uint8_t mac[6]);
// Serialized BLE manager lifecycle; stop only after the NimBLE host exits.
esp_err_t gagent_p4_ble_start(void);
esp_err_t gagent_p4_ble_stop(void);
#ifdef __cplusplus
}
#endif
