#pragma once
#include "gagent/application/application.hpp"
#include "gagent/esp_runtime.hpp"
#include "gagent/esp_mdns.hpp"
#include "gagent/esp_wifi.hpp"
#include "gagent/esp_http.hpp"
#include "gagent/esp_mqtt.hpp"
#include "gagent/esp_ble.hpp"
#include "gagent/esp_airlink.hpp"
#include "gagent/esp_restart.hpp"
#include "gagent/ota_partition.hpp"
#include "gagent/esp_ota_health.hpp"
#include "gagent/module_ota.hpp"
#include "mbedtls_digest.hpp"
#include <variant>
#include "gagent/coredump_upload.hpp"
#ifdef ESP_PLATFORM
#include "gagent/esp_coredump.hpp"
#include "gagent/esp_runtime_logs.hpp"
#include "gagent/esp_clock.hpp"
#endif
namespace gagent::platform {
struct ApplicationConfig {
    runtime::ApplicationConfig application{};
    std::optional<runtime::StartupDefaults> soc_product;
    device::BoardControl control=nullptr;
    uart_port_t uart=UART_NUM_0;
    int tx_pin=-1,rx_pin=-1;
    SerialExtension* serial_extension=nullptr;
    bool enabled=false,allow_migration=false;
    diagnostics::Config coredump{};
    EspMdns::Services mdns_services=nullptr; // Board service records; no second mDNS instance.
    ota::CacheStore* mcu_ota_store=nullptr; // Optional board-owned storage; must outlive EspApplication.
};
enum class ApplicationStart {started,disabled,busy,invalid_configuration,identity_unavailable,ota_unavailable,entropy_failure,uart_failure,worker_failure};
// ESP composition root. Keep in static/long-lived storage: this owns all callback targets,
// fixed business buffers and the only UART/Wi-Fi/HTTP/MQTT/BLE adapters. Boot outlives it.
// Construct/start before board RF/ADC/I2S use; prepare_entropy owns the early SAR entropy phase.
// Board string views/seed fields outlive the application. selected Profile is authoritative.
class EspApplication {
public:
    EspApplication(runtime::Boot&,const board::Profile&,const ApplicationConfig&) noexcept;
    EspApplication(const EspApplication&)=delete;
    EspApplication& operator=(const EspApplication&)=delete;
    ApplicationStart start() noexcept;
    bool post_voice_command(std::uint32_t id) noexcept {return application_.post_voice_command(id);}
    bool start_peripheral_ota(EspHttp::OtaJob job,void* ctx) noexcept {return !module_image_.busy() && http_.begin_ota_job(job,ctx);}
    bool peripheral_ota_available() const noexcept {return !module_image_.busy() && http_.idle();}
    bool peripheral_ota_busy() const noexcept {return http_.ota_job_busy();}
    void cancel_peripheral_ota() noexcept {if(http_.ota_job_busy())http_.cancel();}
    void request_stop() noexcept {worker_.request_stop();}
    // Worker may stop before HTTP/MQTT SDK teardown finishes. Do not destroy/restart while busy.
    bool busy() const noexcept;
    bool running() const noexcept {return worker_.running();}
    runtime::StartupState startup_state() const noexcept {return worker_.startup_state();}
    PreparationState preparation_state() const noexcept {return worker_.preparation_state();}
    // Only on the runtime owner, never an SDK callback or another RTOS task.
    runtime::Application& application() noexcept {return application_;}
private:
    runtime::ApplicationPorts ports() noexcept;
    runtime::Boot& boot_;ApplicationConfig config_;
    runtime::ApplicationConfig effective_;
    EspIo io_;EspWifi wifi_;EspHttp http_;EspMqtt mqtt_;EspBle radio_;EspAirlinkCapture capture_;EspAirlinkUdp udp_;EspRestart restart_;
    EspBootConfirmation boot_confirmation_;EspMdns mdns_;
    OtaPartition ota_partition_;Md5Digest ota_digest_;ota::Cache ota_cache_;
    Md5Digest stream_digest_;ota::Stream ota_stream_;
    Md5Digest module_digest_;ModuleOta module_image_;
    std::variant<std::monostate,device::Soc,device::Mcu> endpoint_;
#ifdef ESP_PLATFORM
    EspRuntimeLogs runtime_logs_;
    EspClock system_clock_;
#endif
    runtime::Application application_;
    std::optional<runtime::McuBootstrap> bootstrap_;
    Worker worker_;
#ifdef ESP_PLATFORM
    EspCoredump coredump_;
#endif
    bool wired_=false;
};
}
