#include "selected_cloud.hpp"
#include "gagent/esp_application.hpp"
#include "board_profile.hpp"
#include "gizwits_product.hpp"
#include "product_config.hpp"
#include "esp_log.h"

namespace gagent::board {
void start(runtime::Boot& boot) noexcept {
    // 不静默将其他产品的已保存设备身份用于当前产品。
    if (const auto* saved = boot.snapshot()) {
        const auto pk = storage::Codec::get(*saved, "pk");
        if (!pk.empty() && pk != datapoints::product_key) {
            ESP_LOGE("gagent", "Stored product differs; see README before switching products");
            return;
        }
    }
    product::initialize();
    static firmware::SelectedCloud cloud;
    static const auto config = [] {
        platform::ApplicationConfig value;
        value.enabled = true;
        value.allow_migration = false;
        value.application.provider = &cloud.provider();
        value.control = product::control;
        value.soc_product = runtime::StartupDefaults{datapoints::product_key,
            configuration::product_secret, configuration::attributes,
            configuration::cloud_host, configuration::cloud_port};
        value.application.bluetooth_name = configuration::bluetooth_name;
        value.application.ota_flash_size = "4M";
        value.application.soc_tick = product::tick;
        value.application.product.configuration_mode = onboarding::Mode::ble;
        value.application.automatic_ota = false;
        for (unsigned i = 0; i < 8; ++i)
            value.application.product.discovery.business_protocol[i] = datapoints::business_protocol[i];
        return value;
    }();
    static platform::EspApplication application(boot, selected, config);
    ESP_LOGI("gagent", "SoC application start=%d", static_cast<int>(application.start()));
}
}
