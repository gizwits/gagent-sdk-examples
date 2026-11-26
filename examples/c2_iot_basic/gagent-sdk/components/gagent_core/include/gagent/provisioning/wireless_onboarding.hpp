#pragma once
#include "gagent/provisioning/airlink.hpp"
#include "gagent/provisioning/softap.hpp"
#include "gagent/network/wifi_scan.hpp"
#include <optional>
namespace gagent::runtime {
// One mode coordinator. Protocol parsing, persistence and confirmation stay in their shared owners.
class WirelessOnboarding final : public onboarding::ModeControl {
public:
    enum class Phase {idle,prepare,radio,decode,drain,release,confirm,trial_ack,trial_prepare,trial_connect,trial_scan,trial_release,reconnect_prepare,reconnect_scan,quarantined};
    WirelessOnboarding(Boot& boot,onboarding::Service& service,wifi::Driver& driver,
        airlink::Decoder& decoder,airlink::Confirmation& confirmation,SoftAp* ap=nullptr) noexcept
        :boot_(boot),service_(service),driver_(driver),decoder_(decoder),confirmation_(confirmation),ap_(ap),trial_(service) {}
    bool start(wifi::Manager&,std::uint64_t now) noexcept;
    onboarding::ModeResult request(onboarding::Mode,std::uint64_t now) noexcept override;
    protocol::Status temporary(const protocol::Context&,const protocol::Frame&,std::uint64_t now) noexcept;
    const wifi::Credential* station_credential() const noexcept {return manager_ && manager_->state()==wifi::State::online?&manager_->current():nullptr;}
    bool temporary_active() const noexcept {return phase_>=Phase::trial_ack && phase_<=Phase::trial_release;}
    void tick(std::uint64_t now) noexcept;
    void stop(std::uint64_t now) noexcept;
    bool running() const noexcept {return running_;}
    void pause_station() noexcept {if(manager_)manager_->stop();}
    void resume_station(std::uint64_t now) noexcept {if(manager_ && boot_.snapshot())manager_->start(*boot_.snapshot(),now);}
    bool restart(std::uint64_t now) noexcept {return manager_ && start(*manager_,now);}
    bool blocks_station() const noexcept {return wanted_ || (phase_!=Phase::idle && phase_!=Phase::confirm);}
    bool busy() const noexcept {return phase_!=Phase::idle;}
    bool failed() const noexcept {return fatal_;}
    bool safe_to_shutdown() const noexcept {return phase_==Phase::idle || phase_==Phase::confirm;}
    Phase phase() const noexcept {return phase_;}
    Boot& boot() noexcept {return boot_;}
    wifi::Driver& driver() noexcept {return driver_;}
    onboarding::Service& onboarding() noexcept {return service_;}
    SoftAp* softap() noexcept {return ap_;}
private:
    bool remember_station() noexcept;
    bool reconnect_tick(std::uint64_t now) noexcept;
    void finish_reconnect(std::uint64_t now,bool cancelled) noexcept;
    bool trial_tick(std::uint64_t now) noexcept;
    void trial_release(std::uint64_t now) noexcept;
    void drain(std::uint64_t now) noexcept;
    void release(std::uint64_t now) noexcept;
    void finish(std::uint64_t now) noexcept;
    bool current(std::uint64_t now) const noexcept;
    Boot& boot_;onboarding::Service& service_;wifi::Driver& driver_;
    airlink::Decoder& decoder_;airlink::Confirmation& confirmation_;SoftAp* ap_;
    std::optional<wifi::ReconnectScan> reconnect_;
    std::uint64_t reconnect_generation_=0,reconnect_window_=0,remembered_completion_=0;
    onboarding::Trial trial_;
    bool trial_failed_=false,trial_scan_needed_=false;
    wifi::Credential trial_current_{};
    wifi::CaptureRadio* radio_=nullptr;wifi::Manager* manager_=nullptr;
    airlink::Captured captured_{};
    std::uint64_t now_=0,window_=0,wanted_window_=0,deadline_=0,retry_at_=0;
    Phase phase_=Phase::idle;
    bool running_=false,wanted_=false,got_=false,fatal_=false,ap_error_=false;
};
}
