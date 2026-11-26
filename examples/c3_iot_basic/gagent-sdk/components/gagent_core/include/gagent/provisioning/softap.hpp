#pragma once
#include "gagent/provisioning/onboarding.hpp"
namespace gagent::runtime {
// Radio/window policy only; all LAN/BLE onboarding parsing and persistence stay in Service.
class SoftAp final : public onboarding::ModeControl {
public:
    SoftAp(wifi::AccessPointDriver& radio,onboarding::Service& service,const wifi::AccessPoint& config) noexcept
        :radio_(radio),service_(service),config_(config) {}
    SoftAp(const SoftAp&)=delete;
    SoftAp& operator=(const SoftAp&)=delete;
    onboarding::ModeResult request(onboarding::Mode,std::uint64_t now) noexcept override;
    bool configure(const wifi::AccessPoint& value) noexcept {
        if(running_ || radio_.ap_state()!=wifi::AccessPointState::stopped || !wifi::valid(value))return false;
        config_=value;return true;
    }
    bool start(std::uint64_t now) noexcept;
    void tick(std::uint64_t now) noexcept;
    // The Worker refreshes this before each tick so a committed LAN ACK can drain.
    void pending_output(bool value) noexcept {pending_output_=value;}
    void suspend(bool value) noexcept {suspended_=value;if(value)ack_deadline_=0;}
    void stop() noexcept {running_=false;ack_deadline_=0;}
    bool running() const noexcept {return running_;}
    bool online() const noexcept {return running_ && desired_ && !stopping_ && !fatal_ && radio_.ap_state()==wifi::AccessPointState::online;}
    bool failed() const noexcept {return fatal_;}
    const std::array<std::uint8_t,4>& address() const noexcept {return config_.address;}
    wifi::AccessPointDriver& radio() noexcept {return radio_;}
    onboarding::Service& onboarding() noexcept {return service_;}
private:
    void stop_cycle(std::uint64_t now,wifi::AccessPointState) noexcept;
    void retry(std::uint64_t now) noexcept;
    wifi::AccessPointDriver& radio_;onboarding::Service& service_;wifi::AccessPoint config_;
    std::uint32_t backoff_ms_=1000;
    std::uint64_t now_=0,completion_=0,window_=0,ack_deadline_=0,deadline_=0,retry_at_=0,stop_at_=0;
    bool pending_output_=false;
    bool suspended_=false,running_=false,desired_=false,cycle_=false,stopping_=false,fatal_=false;
};
}
