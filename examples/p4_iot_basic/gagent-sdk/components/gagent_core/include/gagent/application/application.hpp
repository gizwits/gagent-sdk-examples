#pragma once
#include "gagent/build_features.hpp"
#include "gagent/cloud/provider.hpp"
#include "gagent/cloud/c0.hpp"
#include "gagent/application/preparation.hpp"
#include "gagent/network/mdns.hpp"
#include "gagent/application/mcu_product.hpp"
#include "gagent/network/announcements.hpp"
#include "gagent/cloud/cloud_settings.hpp"
#include "gagent/ota/ota_download.hpp"
#include "gagent/ota/ota_cache.hpp"
#include "gagent/ota/mcu_ota_stream.hpp"
#include "gagent/ota/ota_health.hpp"
#include "gagent/ota/ota_check.hpp"
#include "gagent/ota/ota_attempt.hpp"
#include "gagent/cloud/auth_key.hpp"
#include "gagent/application/product_options.hpp"
#include "gagent/network/bluetooth.hpp"
#include "gagent/provisioning/wireless_onboarding.hpp"
#include "gagent/application/runtime_status.hpp"
#include "gagent/cloud/connectivity.hpp"
#include <optional>
#include <memory>
#include "gagent/application/voice.hpp"
namespace gagent::runtime {
struct ApplicationPorts {
    lan::Transport& local;
    device::Endpoint& device;
    wifi::Driver& wifi;
    wifi::AccessPointDriver& access_point;
    ble::Radio& radio;
    cloud::Http& http;
    mqtt::Driver& mqtt;
    airlink::Decoder& decoder;
    airlink::Transport& confirmation;
    device::Mcu* mcu=nullptr;
    management::Restart* restart=nullptr;
    ota::Cache* mcu_image=nullptr;
    cloud::Download* download=nullptr; // Same physical HTTP adapter as http.
    ota::BootConfirmation* boot_confirmation=nullptr;
    ota::ModuleImage* module_image=nullptr;
    mdns::Publisher* mdns=nullptr;
    ota::Stream* mcu_stream=nullptr; // Optional HTTP streaming destination; BLE keeps the Flash cache.
    diagnostics::RuntimeLogs* runtime_logs=nullptr; // Optional lazy diagnostic capture, no boot-time buffers.
};
struct ApplicationConfig {
    const cloud::Provider* provider=nullptr; // Required; owned by the composition root.
    ProductOptions product{};
    onboarding::Policy onboarding{};
    std::string_view bluetooth_name;
    std::string_view provisioned_auth_key; // Long-lived production input; saved ak takes precedence.
    bool bluetooth_enabled=true,configure_when_empty=true,http_tls=false,mqtt_tls=false;
    storage::Md5 md5=nullptr;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    ble::v5::Hash ble_sha256=nullptr;
    ble::v5::Crypto ble_crypto{};
#endif
    cloud::Cipher encrypt=nullptr,decrypt=nullptr;
    // Runtime-owner callbacks: return already-known versions without querying UART or storage.
    // The result callback must copy metadata before returning; only Gizwits invokes these.
    std::string_view ota_voice_hardware; // Board identity; unknown software uses 00000000 when set.
    ota::FirmwareVersion (*ota_voice_version)() noexcept = nullptr;
    void (*ota_voice_update)(const ota::FirmwareReply&) noexcept = nullptr;
    management::VoicePlay voice_play=nullptr; // Configured before startup, called on MCU worker.
    management::VoiceSetting voice_volume=nullptr,voice_enabled=nullptr,voice_sound=nullptr;
    void (*voice_dialogue_mode)(bool continuous) noexcept=nullptr; // Accepted live handshake; no I/O.
    // Optional SoC product work on the existing runtime owner, after report time advances.
    // Never called for MCU, before activation, or while stopping/resetting/upgrading.
#ifdef GAGENT_CAMERA_ENABLED
    camera::Control* camera=nullptr; // Board-owned, must outlive Application.
#endif
    void (*soc_tick)(reports::Service&, std::uint64_t) noexcept = nullptr;
    // Optional board work on the runtime owner, after cloud startup persistence,
    // MQTT subscriptions and automatic OTA checks finish. Must not block.
    void (*cloud_idle_tick)() noexcept = nullptr;
    bool (*peripheral_ota_busy)() noexcept = nullptr;
    bool (*peripheral_ota_finished)() noexcept = nullptr;
    void (*peripheral_ota_cancel)() noexcept = nullptr;
    ota::BlePeripheral* ble_voice_ota=nullptr; // Board-owned, outlives Application and its OTA I/O task.
    bool defer_bluetooth_until_ota=false;
    // Confirmed cloud updates release BLE before download to preserve TLS memory margin.
    // Availability checks and version reports keep existing BLE connections.
    // Explicit coexistence experiments may opt out; startup order is independent.
    bool suspend_bluetooth_during_cloud_ota=true;
    bool automatic_ota=true;
    bool report_version_changes=true; // Gizwits only; independent of automatic download policy.
    std::uint32_t automatic_ota_delay_ms=10000; // Cloud-ready delay; MQTT/health gates still apply at zero.
    std::string_view ota_flash_size,ota_redmine_id; // Explicit legacy build-report metadata.
    std::string_view ota_sdk_hardware,ota_sdk_software; // Platform build metadata; ESP replaces these with CMake-generated versions.
    identity::Random random=nullptr;
    beacon::Receiver* beacon_receiver=nullptr; // Board-owned, outlives Application and radio teardown.
};
// Shared SoC/MCU business assembly. Ports and board string constants outlive this object.
// Contains large fixed protocol buffers: keep in long-lived storage, never on an RTOS task stack.
// Construction/prepare do no network or radio I/O. One Worker owns every call and service.
class Application final : public Preparation,public NetworkServices,public local::ResetControl,public onboarding::TemporaryControl,public status::OtaActivity,private mqtt::Management,private device::ProductReceiver,private ota::BleBackend,private management::FactoryResetNotice {
public:
    Application(Boot&,const ApplicationPorts&,const ApplicationConfig&) noexcept;
    Application(const Application&)=delete;
    Application& operator=(const Application&)=delete;
    protocol::Status reset(const protocol::Context&,const protocol::Frame&,protocol::Reply&,std::uint64_t now) noexcept override;
    onboarding::TemporaryResult temporary(const protocol::Context&,const protocol::Frame&,std::uint64_t now) noexcept override;
    bool resetting() const noexcept override {return local_restart_.pending() || reset_failed_ || (management_ && management_->factory_reset());}
    enum class ProductRefresh {idle,queued,draining,applied,rejected,failed};
    ProductRefresh product_refresh_state() const noexcept {return refresh_;}
    bool quiescing() const noexcept override {return resetting() || refresh_==ProductRefresh::draining;}
    bool current_discovery(lan::Discovery& value) const noexcept override {if(!prepared_)return false;value=product_.discovery;return true;}
    // Single voice RX task may post; configuration of observer is startup-only.
    bool post_voice_command(std::uint32_t id) noexcept {return voice_.post(id);}
    void voice_observer(VoiceReports::Observer observer) noexcept {voice_.observer(observer);}
    bool valid() const noexcept {return attached_;}
    bool prepare(Engine&,lan::Discovery&) noexcept override;
    bool activated(std::uint64_t now) noexcept override;
    void advance(std::uint64_t now) noexcept override;
    void stopped() noexcept override;
    void boot_tick(std::uint64_t now) noexcept override {if(ota_health_ && (!activated_ || now>=300000))ota_health_->tick(now,false);}
    bool boot_pending() const noexcept override {return ota_health_ && ota_health_->pending();}
    bool drain_stop() noexcept override;
    void connected(std::uint64_t now) noexcept override {router_pending_=router_lost_=false;router_online_revision_=onboarding_.completion_revision();if(!ble_ota_.active())bluetooth_.station_connected(now);mdns_connected_=true;mdns_due_=0;announcements_.connected(now);connectivity_.connected(now);network_progress();}
    void disconnected() noexcept override {mdns_connected_=false;if(ports_.mdns)ports_.mdns->stop();announcements_.disconnected();if(ota_health_)ota_health_->interrupt();if(!ble_ota_.active() && (pushed_update_ || download_origin_ || update_requested_ || automatic_running() || (config_.peripheral_ota_busy && config_.peripheral_ota_busy())))cancel_update();connectivity_.disconnected();progress_cloud_=cloud::State::offline;progress_mqtt_=mqtt::State::offline;progress_recovery_=RecoveryState::idle;}
    void station(wifi::State) noexcept override;
    void tick(std::uint64_t now) noexcept override;
    void station_failure(std::uint64_t,unsigned,bool) noexcept override;
    wifi::RetryPolicy retry_policy() const noexcept override {return {product_.dual_channel?0u:60000u,false};}
    identity::AuthState auth_state() const noexcept {return auth_state_;}
    ProductError product_error() const noexcept {return error_;}
    Engine& engine() noexcept {return engine_;}
    // Composition-root wiring for McuBootstrap, before Worker start. The
    // workspace outlives bootstrap; runtime ownership lasts until stopped().
    mcu::ProductWorkspace& product_workspace() noexcept {return product_workspace_;}
    Bluetooth& bluetooth() noexcept {return bluetooth_;}
    WirelessOnboarding& wireless() noexcept {return wireless_;}
    onboarding::Service& onboarding() noexcept {return onboarding_;}
    lan::AnnouncementState announcement_state() const noexcept {return announcements_.state();}
    binding::Service& binding() noexcept {return binding_;}
    reports::Service& reports() noexcept {return reports_;}
    const ProductOptions& product() const noexcept {return product_;}
    // Worker-only view of the existing cloud-synchronized clock; false before sync.
    bool unix_time(std::uint64_t now,std::uint64_t& seconds) const noexcept {return connectivity_.unix_time(now,seconds);}
    const time::Source& clock() const noexcept {return connectivity_;}
    // Worker-only download sink. Erase/verify progress through tick(), never SDK callbacks.
    bool begin_update(const ota::CheckMetadata&) noexcept;
    bool begin_mcu_update(const ota::CheckMetadata& metadata) noexcept {return metadata.target==ota::Target::mcu && begin_update(metadata);}
    bool begin_module_update(const ota::CheckMetadata& metadata) noexcept {return metadata.target==ota::Target::module && begin_update(metadata);}
    ota::CheckState update_check_state() const noexcept {return check_state_;}
    bool module_ota_selected() const noexcept {return module_selected_;}
    ota::AutoCheckState automatic_update_state() const noexcept {return auto_state_;}
    ota::BuildReportState build_report_state() const noexcept {return build_report_state_;}
    ota::CheckState mcu_update_check_state() const noexcept {return check_state_;}
    bool begin_mcu_download(const cloud::Address&,const cloud::Request&) noexcept;
    bool begin_mcu_image(const ota::Offer&) noexcept;
    bool append_mcu_image(protocol::Bytes) noexcept;
    bool finish_mcu_image() noexcept;
    void cancel_update() noexcept;
    void cancel_mcu_image() noexcept {cancel_update();}
    ota::CacheState mcu_image_state() const noexcept {return ports_.mcu_image?ports_.mcu_image->state():ota::CacheState::empty;}
    bool start_mcu_ota(ota::Image& image,std::uint64_t now) noexcept {
        const bool owned=image_pending_ && (&image==ports_.mcu_image || (stream_selected_ && &image==ports_.mcu_stream));
        const bool streaming=owned && stream_selected_ && &image==ports_.mcu_stream;
        return !ble_ota_.active() && (!config_.ble_voice_ota || !config_.ble_voice_ota->busy()) && (!config_.peripheral_ota_busy || !config_.peripheral_ota_busy()) && !wireless_.temporary_active() && (refresh_!=ProductRefresh::queued || owned) && refresh_!=ProductRefresh::draining && (!automatic_running() || owned) && (!ports_.module_image || !ports_.module_image->busy()) && (!ota_health_ || ota_health_->settled()) && activated_ && !resetting() && !pushed_update_ && !update_requested_ && (streaming || (!download_loan_ && (!image_controller_ || !image_controller_->draining()))) && (!image_pending_ || owned) && product_.profile.mcu_ota && management_ && management_->start_ota(image,now);
    }
    ota::State mcu_ota_state() const noexcept {return management_?management_->ota_state():ota::State::idle;}
    bool ota_active() const noexcept override {return update_active() || automatic_running();}
    ota::HealthState ota_health_state() const noexcept {return ota_health_?ota_health_->state():ota::HealthState::not_required;}
    bool online() const noexcept {return connectivity_.online();}
private:
    void ble_disconnect(std::uint32_t session) noexcept override {ble_.disconnect(session);}
    bool ble_available(ota::Target) const noexcept override;
    ota::ImageSink* ble_sink(ota::Target) noexcept override;
    std::string_view ble_product() const noexcept override;
    std::string_view ble_version(ota::Target) const noexcept override;
    bool ble_install_ready(ota::Target) const noexcept override;
    bool ble_install(ota::Target,std::string_view,std::uint64_t) noexcept override;
    ota::BleInstall ble_install_state(ota::Target) const noexcept override;
    void ble_cancel_install(ota::Target) noexcept override;
    protocol::Status receive(const protocol::Frame&,std::uint64_t now) noexcept override;
    protocol::Status channel_telemetry(const c0::Request& request,std::uint64_t now) noexcept override {return c0_.receive(request,now);}
    protocol::Status update(const mqtt::Update&,std::uint64_t) noexcept override;
    bool product(protocol::Bytes,std::uint64_t) noexcept override;
    bool refresh_product_tick(std::uint64_t) noexcept;
    bool recover_network(std::uint64_t) noexcept;
    void cloud_offline_tick(std::uint64_t) noexcept;
    std::uint64_t cloud_offline_deadline_=0,cloud_offline_window_=0,cloud_offline_revision_=0;
    std::uint64_t router_deadline_=0,router_failure_revision_=0,router_online_revision_=0;
    bool router_pending_=false,router_lost_=false;
    bool stage_product() noexcept;
    ProductRefresh refresh_=ProductRefresh::idle;
    mcu::ProductWorkspace product_workspace_;
    std::uint64_t refresh_deadline_=0,refresh_binding_deadline_=0;
    bool refresh_binding_finite_=false;
    bool refresh_window_=false,refresh_network_=false,refresh_wireless_=false,refresh_ap_=false,refresh_bluetooth_=false;
    void mdns_tick(std::uint64_t now) noexcept;
    bool mdns_connected_=false;
    std::uint64_t mdns_due_=0;
    protocol::Status runtime_log(bool,std::string_view,std::uint64_t) noexcept override;
    void cloud_command_tick(std::uint64_t now) noexcept;
    bool factory_reset_notice(std::uint64_t now) noexcept override;
    std::uint16_t cloud_command_=0;
    cloud::Settings cloud_settings_{};
    void update_request_tick(std::uint64_t now) noexcept;
    void pushed_update_tick(std::uint64_t now) noexcept;
    bool cloud_ota_radio_ready(std::uint64_t now) noexcept;
    void cloud_ota_radio_tick(std::uint64_t now) noexcept;
    // Own MQTT payload only while waiting for asynchronous radio teardown.
    std::unique_ptr<mqtt::Update> pushed_update_;
    bool cloud_ota_radio_held_=false;
    std::uint64_t cloud_ota_radio_deadline_=0;
    bool target_enabled(ota::Target) const noexcept;
    bool queue_update(ota::Target,std::uint64_t) noexcept;
    bool begin_cached_mcu() noexcept;
    bool admit_update(ota::Target,std::string_view version) noexcept;
    ota::AttemptBudget ota_attempts_;
    void automatic_update_tick(std::uint64_t) noexcept;
    void version_report_tick(std::uint64_t) noexcept;
    bool current_versions(ota::CheckMetadata&) const noexcept;
    bool begin_check(const ota::CheckMetadata&,bool report_only) noexcept;
    void version_report_result(bool) noexcept;
    struct VersionSnapshot {
        std::array<std::array<char,33>,6> values{}; // Wi-Fi, MCU, voice: hardware/software pairs.
        std::array<std::uint8_t,16> identity{};
        bool operator==(const VersionSnapshot& other) const noexcept {return values==other.values && identity==other.identity;}
    };
    bool version_snapshot(const ota::CheckMetadata&,VersionSnapshot&) const noexcept;
    VersionSnapshot reported_versions_{},request_versions_{};
    bool versions_reported_=false,versions_requested_=false,version_report_only_=false;
    std::uint64_t version_report_now_=0,version_report_due_=0;
    unsigned version_report_backoff_=5000;
    bool automatic_running() const noexcept {
        return auto_state_==ota::AutoCheckState::running_module || auto_state_==ota::AutoCheckState::waiting_mcu || auto_state_==ota::AutoCheckState::running_mcu;
    }
    bool update_active() const noexcept {
        return ble_ota_.active() || (config_.ble_voice_ota && config_.ble_voice_ota->busy()) || (config_.peripheral_ota_busy && config_.peripheral_ota_busy()) ||
            module_selected_ || pushed_update_ || checked_update_ || update_requested_ || check_pending_ || image_pending_ || download_loan_ ||
            (image_controller_ && image_controller_->draining()) || (management_ && management_->ota_active());
    }
    ota::AutoCheckState auto_state_=ota::AutoCheckState::idle;
    bool startup_ble_released_=false;
    std::uint64_t auto_due_=0;
    std::array<std::uint8_t,16> auto_identity_{};
    bool auto_dispatch_=false;
    ota::BuildReportState build_report_state_=ota::BuildReportState::idle;
    std::array<char,15> pending_build_{};
    bool update_requested_=false;
    std::uint64_t update_request_deadline_=0;
    std::uint32_t update_request_generation_=0;
    std::array<std::uint8_t,16> update_request_identity_{};
    void network_progress() noexcept;
    void image_tick(std::uint64_t now) noexcept;
    bool image_available(ota::Target target=ota::Target::mcu) const noexcept;
    const char* image_unavailable_reason(ota::Target target) const noexcept;
    void select_image(ota::Target,bool http=true) noexcept;
    bool mcu_streaming_requested() const noexcept {
        return ports_.mcu && ports_.mcu->information().ota_streaming();
    }
    ota::ImageSink* mcu_download_sink() const noexcept {
        return mcu_streaming_requested()?static_cast<ota::ImageSink*>(ports_.mcu_stream):ports_.mcu_image;
    }
    bool stream_selected_=false,stream_started_=false;
    ota::Target image_target_=ota::Target::mcu,requested_target_=ota::Target::mcu;
    bool module_selected_=false;
    void release_download_http() noexcept;
    void check_tick() noexcept;
    void checked_update_tick() noexcept;
    // Temporary ownership only between a validated check and download/peripheral dispatch.
    std::unique_ptr<ota::CheckReply> checked_update_;
    bool checked_voice_update_=false;
    bool check_pending_=false;
    ota::CheckState check_state_=ota::CheckState::idle;
    std::array<char,33> check_current_{},check_mcu_current_{},check_voice_current_{};
    ota::FirmwareReply cached_mcu_{};
    std::uint64_t cached_mcu_deadline_=0;
    std::optional<ota::Health> ota_health_;
    std::optional<ota::DownloadController> image_controller_;
    std::uint32_t download_loan_=0;
    bool download_origin_=false;
    bool image_pending_=false;
    wifi::State progress_wifi_=wifi::State::stopped;
    cloud::State progress_cloud_=cloud::State::offline;
    mqtt::State progress_mqtt_=mqtt::State::offline;
    RecoveryState progress_recovery_=RecoveryState::idle;
    void server_restart_tick(std::uint64_t now) noexcept;
    std::uint64_t server_restart_deadline_=0,server_restart_window_=0;
    std::uint32_t server_restart_generation_=0;
    unsigned server_restart_hours_=0;
    RestartScheduler local_restart_;
    bool reset_failed_=false;
    bool reset_notice_sent_=false;
    bool reset_notice_done_=false;
    std::uint64_t reset_notice_started_=0;
    Boot& boot_;ApplicationPorts ports_;ApplicationConfig config_;VoiceReports voice_;ProductOptions product_{};
    lan::Server lan_;ble::Server ble_;ota::BleOta ble_ota_;Engine engine_;
    onboarding::Service onboarding_;binding::Service binding_;lan::Announcements announcements_;
    cloud::Service cloud_;mqtt::Service mqtt_;c0::Service c0_;Connectivity connectivity_;
    reports::Service reports_;std::optional<management::Service> management_;
    Bluetooth bluetooth_;Status status_;SoftAp softap_;
    airlink::Confirmation confirmation_;WirelessOnboarding wireless_;
    std::array<char,61> onboarding_host_{};
    identity::AuthState auth_state_=identity::AuthState::storage_unavailable;
    ProductError error_=ProductError::none;
    bool attached_=false,prepared_=false,activated_=false;
};
}
