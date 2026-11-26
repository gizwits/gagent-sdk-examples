#pragma once
#include "gagent/network/wifi.hpp"
#include "gagent/diagnostics/factory_test.hpp"
#include "esp_event.h"
#include "esp_netif.h"
#include <atomic>
#include <memory>
namespace gagent::platform {
// Exclusively owns one radio for STA and SoftAP; does not coexist with another Wi-Fi owner.
// Long-lived object. Only the Worker calls methods; callbacks only update atomic flags.
class EspWifi final : public wifi::Driver, public wifi::AccessPointDriver, public wifi::CaptureRadio, public factory::Scanner, public wifi::Survey {
public:
    bool begin(const wifi::Credential&) noexcept override;
    bool begin_saved(const wifi::Credential&) noexcept override;
    bool saved_hints() const noexcept override {return true;}
    bool dual_band() const noexcept override;
    bool begin_saved_scan(const wifi::Credential&,wifi::SavedScan,std::uint64_t) noexcept override;
    wifi::Link poll_saved(std::uint64_t) noexcept override;
    bool selects_onboarding_ap() const noexcept override {return true;}
    bool begin_onboarding(const wifi::Credential&,std::uint64_t) noexcept override;
    wifi::Link poll_onboarding(std::uint64_t) noexcept override;
    const wifi::Credential* onboarding_credential() const noexcept override;
    wifi::Link poll() noexcept override;
    bool waiting_for_ip() const noexcept override;
    std::uint64_t ip_wait_deadline() const noexcept override {return dhcp_waiting_ && waiting_for_ip()?dhcp_deadline_:0;}
    wifi::Observation observation() const noexcept override;
    const wifi::ScanList* access_points() const noexcept override {return &access_points_;}
    void onboarding_scan(bool,std::uint64_t) noexcept override;
    void end() noexcept override;
    void shutdown() noexcept override;
    bool radio_idle() const noexcept override {return (onboarding_stage_==OnboardingStage::idle || onboarding_stage_==OnboardingStage::failed) && !background_ && !scan_owned_.load() && !capture_ && !radio_started_ && !started_ && !stopping_ && !pending_ && !ap_started_ && !ap_stopping_;}
    wifi::AccessPointDriver* access_point() noexcept override {return this;}
    bool prepare_ap(const wifi::AccessPoint&) noexcept override;
    bool start_ap() noexcept override;
    wifi::AccessPointState poll_ap() noexcept override;
    wifi::AccessPointState ap_state() const noexcept override;
    bool end_ap() noexcept override;
    wifi::CaptureRadio* capture_radio() noexcept override {return this;}
    bool start_capture() noexcept override;
    wifi::CaptureRadioState poll_capture() noexcept override;
    bool stop_capture() noexcept override;
    bool scan_busy() const noexcept {return scan_owned_.load();}
    wifi::Survey* survey() noexcept override {return this;}
    bool start_scan(std::uint64_t now) noexcept override {return start_scan_cycle({},now);}
    wifi::ScanState poll_scan(std::uint64_t now) noexcept override {return poll(now);}
    wifi::ScanState scan_state() const noexcept override {return scan_state_;}
    void stop_scan(std::uint64_t now) noexcept override {stop(now);}
    const wifi::ScanList& scan_results() const noexcept override {return access_points_;}
    factory::Scanner* scanner() noexcept override {return this;}
    bool start(std::string_view target,std::uint64_t now) noexcept override;
    factory::ScanState poll(std::uint64_t now) noexcept override;
    bool result(factory::ScanResult&) const noexcept override;
    void stop(std::uint64_t now) noexcept override;
    EspWifi()=default;
    EspWifi(const EspWifi&)=delete;
    EspWifi& operator=(const EspWifi&)=delete;
private:
    wifi::Link poll_ip(std::uint64_t now) noexcept;
    bool begin_attempt(const wifi::Credential&,bool all_channels,std::uint8_t channel=0) noexcept;
    void end_station() noexcept;
    void retry_or_advance_candidate() noexcept;
    bool start_background(std::uint64_t) noexcept;
    bool collect_background(std::uint64_t) noexcept;
    void prepare_cache(std::size_t) noexcept;
    void cache_entry(wifi::ScanEntry) noexcept;
    bool cached_scan(std::uint64_t) const noexcept;
    void add_onboarding_candidate(const wifi::ScanEntry&) noexcept;
    enum class OnboardingStage {idle,select,connect,online,failed};
    OnboardingStage onboarding_stage_=OnboardingStage::idle;
    wifi::Credential onboarding_request_{},onboarding_resolved_{};
    wifi::ScanList candidates_{};
    std::size_t candidate_index_=0;
    unsigned onboarding_scans_=0;
    bool candidate_retried_=false;
    bool candidates_ready_=false,cache_complete_=false,cache_truncated_=false,background_wait_stop_=false;
    bool had_candidates_=false,onboarding_fallback_=false;
    // A single-band saved-network survey reuses candidate/STOP/drain handling.
    // Its scan bitmap is local to that scan; the radio always remains AUTO.
    bool saved_selection_=false;
    wifi::Band scan_band_=wifi::Band::unknown;
    std::uint8_t selected_channel_=0;
    std::uint64_t onboarding_now_=0,onboarding_deadline_=0,candidate_deadline_=0,onboarding_started_=0;
    std::uint64_t dhcp_started_=0,dhcp_deadline_=0;
    unsigned dhcp_failures_=0;
    bool dhcp_waiting_=false,dhcp_reported_=false;
    bool start_scan_cycle(std::string_view,std::uint64_t) noexcept;
    bool launch_capture() noexcept;
    wifi::CaptureRadioState capture_poll() noexcept;
    bool release_capture() noexcept;
    bool initialize() noexcept;
    bool launch() noexcept;
    bool drain_background() noexcept;
    unsigned background_=0; // 0 idle, 1 scanning, 2 draining callbacks, 3 quarantined
    std::uint64_t background_now_=0,background_due_=0,cache_at_=0;
    bool channel_hint_=false,all_channels_=false;
    // First 32 entries also serve the bounded legacy AP-list response. Overflow
    // extends the same snapshot to 64/128 records without duplicating that prefix.
    static constexpr std::size_t cache_limit=128;
    std::unique_ptr<wifi::ScanEntry[]> cache_overflow_;
    std::size_t cache_overflow_capacity_=0,cache_overflow_count_=0;
    static void event(void*,esp_event_base_t,std::int32_t,void*) noexcept;
    enum : unsigned {started=1,stopped=2,lost=4,got_ip=8,associated=16};
    esp_netif_t* netif_=nullptr;
    esp_netif_t* ap_netif_=nullptr;
    esp_event_handler_instance_t wifi_handler_=nullptr,ip_handler_=nullptr;
    wifi::Credential credential_{};
    wifi::Observation observed_{};
    std::atomic<unsigned> events_{0},ap_events_{0};
#ifdef GAGENT_CONNECTION_TIMING
    std::atomic<std::uint32_t> connect_at_ms_{0},associated_at_ms_{0};
#endif
    wifi::AccessPointState ap_state_=wifi::AccessPointState::stopped;
    std::array<std::uint8_t,4> ap_address_{};
    bool radio_started_=false,ap_prepared_=false,ap_started_=false,ap_stopping_=false,ap_failed_=false;
    std::uint32_t online_ip_=0;
    bool initialized_=false,driver_owned_=false,started_=false,stopping_=false;
    bool pending_=false,connecting_=false,failed_=false,capture_=false;
    std::atomic<bool> scan_owned_{false};
    bool scan_launched_=false,scan_posted_=false;
    std::uint64_t scan_now_=0,scan_deadline_=0;
    unsigned scan_epoch_=0;
    std::atomic<unsigned> scan_event_{0},scan_drained_{0};
    esp_event_handler_instance_t scan_handler_=nullptr;
    std::array<char,33> scan_target_{};
    factory::ScanResult scan_result_{};
    wifi::ScanList access_points_{};
    factory::ScanState scan_state_=factory::ScanState::idle;
    wifi::CaptureRadioState capture_state_=wifi::CaptureRadioState::stopped;
};
}
