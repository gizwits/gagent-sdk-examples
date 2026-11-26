#include "gagent/storage/boot.hpp"
#include "gagent/partition_store.hpp"
#include "mbedtls_md5.hpp"
#include "board_profile.hpp"
#include "esp_log.h"

// 两份 GData 保存设备身份和配网信息；对象须覆盖 SDK 的整个生命周期。
extern "C" void app_main() {
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_WARN);
    const auto* primary = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "GData_0");
    const auto* backup = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, "GData_1");
    if (!primary || !backup) {
        ESP_LOGE("gagent", "Missing GData partitions; use the supplied partition table");
        return;
    }
    static gagent::platform::PartitionStore flash(primary, backup, gagent::platform::Access::read_write);
    static gagent::runtime::Boot boot(flash, gagent::platform::md5);
    const auto state = boot.inspect();
    ESP_LOGI("gagent", "Storage state=%d", static_cast<int>(state));
    gagent::board::start(boot);
}
