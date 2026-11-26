#pragma once
#include "gagent/network/ble_protocol.hpp"
#include "gagent/ota/ota_check.hpp"
#include "gagent/ota/ota_sink.hpp"
#include "gagent/ota/ota_status.hpp"
namespace gagent::ota {
enum class BleInstall {pending,complete,failed};
// Optional board-owned voice sink. Installation runs asynchronously; cancel()
// must retain resources until its I/O owner drains. No ESP dependency in core.
class BlePeripheral : public ImageSink {
public:
    virtual bool available() const noexcept=0;
    virtual std::string_view version() const noexcept=0;
    virtual bool install_ready() const noexcept {return true;}
    virtual bool install(std::string_view) noexcept=0;
    virtual BleInstall install_state() const noexcept=0;
};
class BleBackend {
public:
    virtual ~BleBackend()=default;
    virtual void ble_disconnect(std::uint32_t) noexcept {}
    virtual bool ble_available(Target) const noexcept=0;
    virtual ImageSink* ble_sink(Target) noexcept=0;
    virtual std::string_view ble_product() const noexcept=0;
    virtual std::string_view ble_version(Target) const noexcept=0;
    virtual bool ble_install_ready(Target) const noexcept {return true;}
    virtual bool ble_install(Target,std::string_view,std::uint64_t) noexcept=0;
    virtual BleInstall ble_install_state(Target) const noexcept=0;
    virtual void ble_cancel_install(Target) noexcept=0;
};
// App BLE V2 protocol (0x41..0x52), on the runtime worker. No HTTP, task or
// whole-file allocation. Authentication and connection lifetime come from Server.
class BleOta {
public:
    static constexpr std::size_t block_size=512;
    BleOta(BleBackend& backend,protocol::Reply& reply) noexcept:backend_(backend),reply_(reply) {}
    static bool handles(std::uint16_t c) noexcept {
#if GAGENT_BLE_PROTOCOL_VERSION == 5
        return GAGENT_BLE_V5_OTA && (c==0x20 || c==0x22 || c==0x25 || c==0x2f);
#else
        return c==0x41 || c==0x43 || c==0x45 || c==0x47 || c==0x49;
#endif
    }
    protocol::Status receive(const protocol::Context&,const protocol::Frame&,std::uint64_t) noexcept;
    void tick(std::uint64_t) noexcept;
    void cancel() noexcept;
    void disconnected(std::uint32_t id) noexcept {if(context_.session==id)cancel();}
    bool active() const noexcept {return state_!=State::idle && state_!=State::done;}
    bool status(StatusSnapshot& out) const noexcept {
        if(state_==State::idle)return false;
        out={};out.type=target_==Target::voice?StatusType::voice:target_==Target::mcu?StatusType::mcu:StatusType::module;
        out.progress=overall_progress(offer_.size,received_,0,target_!=Target::module);
        switch(state_){
        case State::reserved:case State::preparing:out.phase=Phase::preparing;break;
        case State::receiving:out.phase=Phase::downloading;break;
        case State::verifying:out.phase=Phase::verifying;break;
        case State::installing:out.phase=Phase::transferring;break;
        case State::done:out.phase=result_?Phase::failed:Phase::confirming;out.reason=result_?Reason::unknown:Reason::none;break;
        default:return false;
        }
        return true;
    }
    bool owns(std::uint32_t id) const noexcept {return active() && context_.session==id;}
private:
    enum class State {idle,reserved,preparing,receiving,verifying,installing,done};
    protocol::Status send(std::uint16_t,protocol::Bytes) noexcept;
    protocol::Status pre_reply(std::uint8_t) noexcept;
    protocol::Status ack(std::uint16_t,std::uint8_t) noexcept;
    protocol::Status finish(std::uint8_t,std::uint64_t) noexcept;
    bool begin() noexcept;
    BleBackend& backend_;protocol::Reply& reply_;
    protocol::Context context_{protocol::Channel::ble,0,false,{}};
    ImageSink* sink_=nullptr;
    State state_=State::idle;Target target_=Target::module;Offer offer_{};
    std::array<char,8> version_{};
    std::array<std::uint8_t,block_size> last_{};
    std::uint32_t received_=0,crc_=0xffffffffu,expected_crc_=0;
    std::uint16_t sequence_=0,last_sequence_=0;
    std::size_t last_size_=0;
    std::uint64_t activity_=0,started_=0,wait_sent_=0;
    std::uint8_t result_=4;
    bool waiting_=false;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    std::uint8_t request_id_=0,end_id_=0,cycle_seq_=0,cycle_total_=15,retries_=0;
    std::uint32_t cycle_received_=0;
    std::uint64_t ack_at_=0;
    protocol::Status v5_ack(std::uint64_t) noexcept;
#endif
};
}
