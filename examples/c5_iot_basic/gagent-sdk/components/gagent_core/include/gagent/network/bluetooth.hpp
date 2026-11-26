#pragma once
#include "gagent/application/runtime.hpp"
namespace gagent::runtime {
// One Worker owns the radio lifecycle and local identity refresh. No separate business task.
class Bluetooth final : public ble::Control, public beacon::Control {
public:
    Bluetooth(Boot& boot,Engine& engine,ble::Server& server,ble::Radio& radio,onboarding::Service& onboarding,
              storage::Md5 md5,const lan::Discovery& discovery,std::string_view name,bool initially_enabled=true) noexcept;
    bool attach() noexcept;
    bool configure_beacon(beacon::Receiver*,bool capable,bool initial) noexcept;
    bool beacon_request(const beacon::Request&,std::uint64_t) noexcept override;
    beacon::State beacon_state() const noexcept override {return beacon_capable_?radio_.scan_state():beacon::State::failed;}
    bool configure(const lan::Discovery&,bool skip_login,bool dual_channel=true,std::string_view name={}) noexcept;
    // Worker-delivered edges; never repeatedly override an explicit MCU command in tick().
    void station_connected(std::uint64_t now) noexcept {if(!dual_channel_ && running_ && now>=now_){window_revision_=onboarding_.window_revision();set_enabled(false,now);}}
    void cloud_connected(std::uint64_t now) noexcept {if(dual_channel_)set_enabled(true,now);}
    bool start(std::uint64_t now) noexcept;
    // Holds initial non-provisioning radio allocation. A provisioning window
    // permanently releases this hold; never tear down an active provisioner.
    void hold_startup(bool hold) noexcept {startup_hold_=hold;}
    // Release the whole radio for OTA without changing MCU/control or beacon intent.
    // busy() stays true until the asynchronous radio teardown has completed.
    void suspend(bool suspended,std::uint64_t now) noexcept;
    bool suspended() const noexcept {return suspended_;}
    void tick(std::uint64_t now) noexcept;
    void stop() noexcept;
    bool desired() const noexcept {return desired_;}
    bool restart(std::uint64_t now,bool enabled) noexcept {const auto initial=initial_;initial_=enabled;restoring_=true;const bool result=start(now);restoring_=false;initial_=initial;return result;}
    bool busy() const noexcept {return radio_.busy();}
    bool set_enabled(bool,std::uint64_t now) noexcept override;
    ble::State state() const noexcept override {return state_==ble::State::enabled && !enabled()?ble::State::failed:state_;}
    bool enabled() const noexcept override {return running_ && desired_ && radio_.ready() && radio_.control_active() && engine_.bluetooth_active();}
    bool connected(std::uint64_t now) const noexcept override {return enabled() && server_.authenticated_clients(now)>0;}
    std::size_t packet_capacity() const noexcept {return server_.packet_capacity();}
private:
    void failed(std::uint64_t now,bool fatal=false) noexcept;
    Boot& boot_;Engine& engine_;ble::Server& server_;ble::Radio& radio_;onboarding::Service& onboarding_;
    storage::Md5 md5_;lan::Discovery discovery_;
    std::array<char,32> name_{},product_key_{};
    std::array<std::uint8_t,16> fingerprint_{};
    std::uint64_t now_=0,generation_=0,window_revision_=0,retry_at_=0;
    std::uint32_t retry_ms_=1000;
    ble::State state_=ble::State::disabled;
    beacon::Receiver* beacon_receiver_=nullptr;
    beacon::Filter beacon_filter_;
    beacon::Request beacon_request_{};
    bool beacon_capable_=false,beacon_initial_=false,beacon_desired_=false,beacon_restore_=false;
    bool dual_channel_=true,restoring_=false;
    bool startup_hold_=false,suspended_=false;
    bool initial_=true,attached_=false,running_=false,desired_=false,cycle_=false,fatal_=false,window_=false,valid_name_=false;
};
}
