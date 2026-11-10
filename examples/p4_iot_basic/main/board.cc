#include "selected_cloud.hpp"
#include "gagent/esp_application.hpp"
#include "board_profile.hpp"
#include "gizwits_product.hpp"
#include "product_config.hpp"
#include "esp_log.h"
#include "sdkconfig.h"
#ifdef GAGENT_CAMERA_ENABLED
#include "gagent/camera/service.hpp"
#include "gagent/camera/config.h"
#endif
namespace gagent::board {
// 装配产品身份、云服务和数据点回调。通常只需修改 product_config.hpp 和业务回调。
void start(runtime::Boot& boot) noexcept {
    // 同一块板可能曾运行其他产品：不要静默沿用不同 PK 的设备身份。
    if (const auto* saved = boot.snapshot()) {
        const auto pk = storage::Codec::get(*saved, "pk");
        if (!pk.empty() && pk != datapoints::product_key) {
            ESP_LOGE("sdk_basic", "Stored product differs from template; back up device data and explicitly clear GData_0/GData_1 before testing");
            return;
        }
    }
    product::initialize();
    // SDK 会保存这些对象的引用；使用静态存储，不能放在短生命周期栈帧里。
    static firmware::SelectedCloud cloud;
    static const auto config = [] {
        platform::ApplicationConfig value;
        value.application.provider=&cloud.provider();
#ifdef GAGENT_CAMERA_ENABLED
        // 摄像头生命周期和联网状态由 SDK 管理，业务层只提交开始/停止意图。
        value.application.camera=&camera::service();
#endif
        value.enabled = configuration::enabled;
        value.allow_migration = configuration::allow_migration;
        value.control = product::control;
        value.soc_product = runtime::StartupDefaults{datapoints::product_key,
            configuration::product_secret, configuration::attributes,
            configuration::cloud_host, configuration::cloud_port};
        value.application.bluetooth_name = configuration::bluetooth_name;
        // 云端容量字段使用 "16M"，IDF 镜像配置使用 "16MB"；容量属于客户工程。
        constexpr std::string_view flash = CONFIG_ESPTOOLPY_FLASHSIZE;
        value.application.ota_flash_size = flash.substr(0, flash.size()-1);
        value.application.soc_tick = product::tick;
        // 无 Wi-Fi 配置时进入 BLE 配网；已有配置时 SDK 自动重连。
        value.application.product.configuration_mode = onboarding::Mode::ble;
        // 基础收发测试不自动下载 OTA，保留模组 OTA 和启动确认能力。
        value.application.automatic_ota = false;
        for (unsigned i=0; i<8; ++i)
            value.application.product.discovery.business_protocol[i] = datapoints::business_protocol[i];
        return value;
    }();
    static platform::EspApplication application(boot, selected, config);
    product::attach(application.application(), boot);
#ifdef GAGENT_CAMERA_ENABLED
    // 可选启动自检：检测传感器并验证原生模式，不开始网络推流。
    // 更改运行参数时，在此调用前使用 gagent_camera_configure()。
    gagent_camera_probe_result_t camera_info{};
    const int camera_error = gagent_camera_probe(&camera_info);
    ESP_LOGI("sdk_basic", "Camera probe=%d native=%lux%lu@%lu output=%lux%lu@%lu", camera_error,
        (unsigned long)camera_info.capture.width, (unsigned long)camera_info.capture.height,
        (unsigned long)camera_info.capture.fps_num, (unsigned long)camera_info.output.width,
        (unsigned long)camera_info.output.height, (unsigned long)camera_info.output.fps_num);
#endif
    const auto result = application.start();
    ESP_LOGI("gagent", "SoC application start=%d", static_cast<int>(result));
}
}
