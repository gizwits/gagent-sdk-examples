#pragma once
#ifdef GAGENT_CAMERA_ENABLED
#include "gagent/camera/control.hpp"
#endif
#include "gagent/storage/boot.hpp"
#include "gagent/ota/mcu_ota_session.hpp"
#include "gagent/application/restart.hpp"
#include "gagent/protocol/reports.hpp"
#include "gagent/cloud/time.hpp"
#include "gagent/provisioning/onboarding.hpp"
#include "gagent/provisioning/binding.hpp"
#include "gagent/application/status.hpp"
#include "gagent/network/ble.hpp"

namespace gagent::management {
using Restart=runtime::Restart;
class FactoryResetNotice {
public:
    virtual ~FactoryResetNotice()=default;
    // Nonblocking, called while the MCU ACK continues draining. True releases reboot.
    virtual bool factory_reset_notice(std::uint64_t now) noexcept=0;
};
// Copies/submits the four raw event bytes; returns the voice protocol result code.
using VoicePlay = std::uint8_t (*)(protocol::Bytes) noexcept;
// Backend receives a validated absolute volume (0..100) or enabled (0/1).
// Return the protocol result; success follows the product's confirmation rule
// (AISpeech has no readback: submitted to UART; mic and sound are independent).
using VoiceSetting = std::uint8_t (*)(std::uint8_t) noexcept;
struct Module {
    std::array<std::uint8_t,8> hardware{}, software{};
    std::array<std::uint8_t,6> mac{};
    std::int32_t default_timezone = 0;
    std::string_view build_time; // Long-lived board/build metadata; empty values remain empty.
    bool time_push = false; // Legacy product ntpPushEnable is opt-in.
};
// BE16 text length followed by legacy module-check fields; no terminating NUL on wire.
bool module_check_payload(const storage::Snapshot&,const Module&,local::Encrypt,std::array<std::uint8_t,256>&,std::size_t&) noexcept;
// One MCU command owner; reports retain their common LAN/MQTT/BLE service.
// Board/platform supplies module versions and MAC, never inferred from another product.
class Service final : public device::McuHandler, private device::McuCompletion {
public:
    Service(device::Mcu& mcu, reports::Service& reports, runtime::Boot& boot,
            const time::Source& clock, const Module& module,local::Encrypt encrypt=nullptr) noexcept
        : mcu_(mcu), ota_(mcu), reports_(reports), boot_(boot), clock_(clock), module_(module),encrypt_(encrypt) {}
    Service(const Service&) = delete;
    Service& operator=(const Service&) = delete;
    bool attach() noexcept;
    // Worker-only entry; the verified image must remain immutable while active.
    bool start_ota(ota::Image&,std::uint64_t now) noexcept;
    bool ota_active() const noexcept {return ota_.active();}
    ota::State ota_state() const noexcept {return ota_.state();}
    void stop_ota() noexcept {ota_.stop();}
    bool module(const Module& value) noexcept {
        if(mcu_.available() || count_ || restart_.pending() || value.default_timezone < -43200 || value.default_timezone > 43200)return false;
        module_=value;return true;
    }
    // Configure the shared state source before activation, on the worker owner.
    void status(const status::Source& source) noexcept {status_=&source;}
    // Coalesces clock updates, then encodes the newest valid clock on the next free transaction.
    void sync_time() noexcept {if(module_.time_push)time_pending_=true;}
    void bluetooth(ble::Control& control) noexcept {bluetooth_=&control;}
    void beacon(beacon::Control& control) noexcept {beacon_=&control;}
    void binding(binding::Service& service) noexcept {binding_=&service;}
#ifdef GAGENT_CAMERA_ENABLED
    void camera(camera::Control* value) noexcept {camera_.attach(value);}
#endif
    void voice_play(VoicePlay play) noexcept {voice_play_=play;}
    void voice_settings(VoiceSetting volume,VoiceSetting enabled,VoiceSetting sound=nullptr) noexcept {voice_volume_=volume;voice_enabled_=enabled;voice_sound_=sound;}
    void products(device::ProductReceiver& receiver) noexcept {products_=&receiver;}
    bool product(protocol::Bytes bytes,std::uint64_t now) noexcept override {return !restart_.pending() && products_ && products_->product(bytes,now);}
    void ready(std::uint64_t now) noexcept override;
    void link_failed(std::uint64_t now) noexcept override;
    bool startup_failed(std::uint64_t now) noexcept override;
    bool restart_complete() const noexcept override {return restart_.fired();}
    bool commands_pending() const noexcept override {return count_ || restart_.pending();}
    bool baud_change_allowed() const noexcept override {
        status::State state;
        return !ota_.active() && !commands_pending() && (!status_ || !status_->read(now_,state) || !state.ota);
    }
    void restart(Restart& action) noexcept {restart_.action(action);}
    void factory_reset_notice(FactoryResetNotice& notice) noexcept {reset_notice_=&notice;}
    bool restarting() const noexcept {return restart_.pending();}
    // MCU 0x0B factory reset only. A plain 0x29 restart does not notify the cloud.
    bool factory_reset() const noexcept {return restart_.pending() && restart_command_==0x0b && (restart_flags_==0 || restart_flags_==3);}
    void onboarding(onboarding::Service& service) noexcept { onboarding_ = &service; }
    void address(const std::array<std::uint8_t,4>& ipv4, bool connected) noexcept { ipv4_ = ipv4; connected_ = connected; }
    void receive(const mcu::Frame&, std::uint64_t now) noexcept override;
    void advance(std::uint64_t now) noexcept override;
    void tick(std::uint64_t now) noexcept override;
    void stopped() noexcept override;
    std::size_t queued() const noexcept { return count_; }
private:
#ifdef GAGENT_CAMERA_ENABLED
    camera::Protocol camera_;
#endif
    FactoryResetNotice* reset_notice_=nullptr;
    VoicePlay voice_play_=nullptr;
    VoiceSetting voice_volume_=nullptr,voice_enabled_=nullptr,voice_sound_=nullptr;
    // Shared across the four voice commands on every chip. New transactions
    // evict the oldest insertion; retries neither refresh order nor expiry.
    struct VoiceResult {
        std::uint64_t expires=0;
        std::array<std::uint8_t,4> bytes{};
        std::uint8_t command=0,sequence=0,result=0;
    };
    static_assert(sizeof(VoiceResult)==16);
    std::array<VoiceResult,4> voice_history_{};
    std::uint8_t voice_result(const mcu::Frame&,std::uint64_t,bool& cached) noexcept;
    device::ProductReceiver* products_=nullptr;
    struct Response {
        // Fixed module information + legacy Aliyun SSID/password length fields.
        std::array<std::uint8_t,65+1+32+1+64> payload{};
        std::uint64_t deadline=0;
        bool beacon_pending=false,radio_pending=false,radio_target=false,inspection=false;
        std::size_t size = 0;
        std::uint8_t command = 0, sequence = 0, request = 0;
        std::uint16_t flags = 0;
    };
    bool information(Response&) const noexcept;
    bool timestamp(Response&,bool require_valid=false) const noexcept;
    void notify() noexcept;
    void completed(device::McuRequest,bool,protocol::Bytes) noexcept override;
    ble::Control* bluetooth_=nullptr;
    beacon::Control* beacon_=nullptr;
    const status::Source* status_=nullptr;
    std::array<std::uint8_t,2> last_status_{};
    std::uint64_t status_sent_at_=0,clock_revision_=0;
    bool status_sent_=false,notification_pending_=false,time_pending_=false;
    bool prefer_time_=false;
    void flush() noexcept;
    std::uint8_t request_mode(std::uint8_t mode,std::uint8_t sequence) noexcept;
    struct ModeRequest {std::uint64_t at=0;std::uint8_t mode=0,sequence=0;bool valid=false;};
    std::array<ModeRequest,4> modes_{};
    device::Mcu& mcu_;
    ota::Session ota_;
    reports::Service& reports_;
    runtime::Boot& boot_;
    const time::Source& clock_;
    Module module_;
    local::Encrypt encrypt_=nullptr;
    std::array<std::uint8_t,256> inspection_{};
    bool inspection_ready_=false;
    binding::Service* binding_=nullptr;
    std::uint64_t permission_accepted_at_=0;
    std::uint8_t permission_sequence_=0,permission_value_=0;
    bool permission_accepted_=false;
    std::uint64_t bind_accepted_at_=0;
    std::uint8_t bind_sequence_=0;
    bool bind_accepted_=false;
    runtime::RestartScheduler restart_;
    std::uint8_t restart_command_=0,restart_sequence_=0;
    std::uint16_t restart_flags_=0;
    onboarding::Service* onboarding_ = nullptr;
    std::array<std::uint8_t,4> ipv4_{};
    std::array<Response,4> responses_{};
    std::size_t head_ = 0, count_ = 0;
    std::uint64_t now_ = 0;
    bool connected_ = false;
};
}
