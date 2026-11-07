#include "gagent/storage/boot.hpp"
#include "board_profile.hpp"
#include "gagent/partition_store.hpp"
#include "mbedtls_md5.hpp"
#include "esp_log.h"
#ifdef ESP_PLATFORM
#include "gagent_version.hpp"
#include "esp_app_desc.h"
#include "esp_mac.h"
#endif
#if GAGENT_COMPACT_2M
extern "C" esp_err_t gagent_no_tls_init();
#endif

#ifdef ESP_PLATFORM
namespace {
void log_identity() {
    const auto log_mac = [](const char* label, esp_mac_type_t type) {
        uint8_t address[6]{};
        const auto result = esp_read_mac(address, type);
        if (result == ESP_OK)
            ESP_LOGI("gagent", "%s: " MACSTR, label, MAC2STR(address));
        else
            ESP_LOGW("gagent", "%s: unavailable (%s)", label, esp_err_to_name(result));
    };
#if CONFIG_IDF_TARGET_ESP32P4
    log_mac("baseMAC", ESP_MAC_BASE);
#else
    log_mac("mac", ESP_MAC_WIFI_STA);
    log_mac("bleMAC", ESP_MAC_BT);
#endif
    ESP_LOGI("gagent", "software_version: %s", gagent::firmware::software_version);
    ESP_LOGI("gagent", "hardware_version: %s", gagent::firmware::hardware_version);
    const auto* description = esp_app_get_description();
    ESP_LOGI("gagent", "build_time: %s %s", description->date, description->time);
}
}
#endif

#ifdef GAGENT_CAMERA_DIAGNOSTIC
void camera_diagnostic_start();
#endif
// ESP-IDF 入口：先准备双副本配置存储，再交给 board::start 装配应用。
extern "C" void app_main() {
#ifdef GAGENT_CAMERA_DIAGNOSTIC
    camera_diagnostic_start();
    return;
#endif
#ifdef ESP_PLATFORM
    log_identity();
#endif
#if GAGENT_COMPACT_2M
    if (gagent_no_tls_init() != ESP_OK) {
        ESP_LOGE("gagent", "Plain TCP backend registration failed");
        return;
    }
#endif
#if CONFIG_LOG_DEFAULT_LEVEL >= 4
    // 避免底层 HTTP 调试日志打印请求凭据，业务诊断使用 SDK 自身的日志。
    esp_log_level_set("HTTP_CLIENT", ESP_LOG_WARN);
#endif
    const auto* primary = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS,"GData_0");
    const auto* backup = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS,"GData_1");
    // 两份 GData 用于配置恢复；对象必须覆盖 SDK 的整个生命周期。
    static gagent::platform::PartitionStore flash(primary,backup,
        gagent::board::selected.storage_writable?gagent::platform::Access::read_write:gagent::platform::Access::read_only);
    static gagent::runtime::Boot boot(flash,gagent::platform::md5);
    const auto state=boot.inspect();
    // 仅打印存储状态，不打印产品密钥、Wi-Fi 密码或原始配置内容。
    ESP_LOGI("gagent", "startup: writable=%d state=%d available=%d",
             gagent::board::selected.storage_writable,static_cast<int>(state),state==gagent::runtime::BootState::ready);
    gagent::board::start(boot);
    // 此处只检查存储；首次身份初始化、联网及配网由应用启动流程统一处理。
}
