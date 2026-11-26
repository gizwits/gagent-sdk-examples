#pragma once
#include "gagent/protocol/device.hpp"
namespace gagent::reports {
// Shared report semantics. Channels only enumerate authenticated recipients and copy frames.
class Service final : public device::McuHandler,public device::Acknowledgements {
public:
    Service(device::Router& router,device::Mcu* mcu,storage::Md5 md5) noexcept : router_(router),mcu_(mcu),md5_(md5) {}
    Service(const Service&)=delete;
    Service& operator=(const Service&)=delete;
    bool attach() noexcept {if(!router_.acknowledgements(*this))return false;if(mcu_)mcu_->handler(*this);return true;}
    void receive(const mcu::Frame&,std::uint64_t now) noexcept override;
    void advance(std::uint64_t now) noexcept override {if(now>=now_)now_=now;}
    void tick(std::uint64_t now) noexcept override;
    void stopped() noexcept override {pending_={};}
    protocol::Status acknowledge(const protocol::Context&,const protocol::Frame&) noexcept override;
    void cloud_enabled(bool enabled) noexcept {cloud_enabled_=enabled;}
    // SoC boards share the encoder/fanout. True means an attempt, not client receipt.
    // Legacy flags: 0x40 LAN+BLE, 0x80 MQTT, 0xc0 both. No destination is rejected.
    // Low bits retain application flags; bit 0 requires MCU acknowledgement ownership.
    // All calls run on the runtime owner; advance time before dispatching client ACKs.
    bool publish(protocol::Bytes,std::uint8_t flags=0xc0) noexcept;
    // Same authenticated 0x91 encoder, scoped to one local transport. Never sends to MQTT.
    bool publish_local(protocol::Bytes,protocol::Channel) noexcept;
private:
    enum class State { empty, waiting, reply, done };
    struct Pending {
        std::array<device::Broadcast,3> targets{};
        std::array<std::uint8_t,16> digest{};
        std::uint64_t deadline=0,finished=0;
        std::uint32_t sequence=0;
        std::uint8_t serial_sequence=0,flags=0,result=0;
        protocol::PayloadFormat format=protocol::PayloadFormat::binary;
        State state=State::empty;
    };
    bool send(protocol::Bytes,std::uint8_t flags,std::uint32_t& sequence,std::array<device::Broadcast,3>&,device::ReportScope override_scope=device::ReportScope::none,protocol::PayloadFormat=protocol::PayloadFormat::binary) noexcept;
    void finish(Pending&,std::uint8_t result) noexcept;
    void flush(Pending&) noexcept;
    device::Router& router_;device::Mcu* mcu_;storage::Md5 md5_;
    std::array<Pending,8> pending_{};
    std::array<std::uint8_t,mcu::payload_limit+4> payload_{};
    std::uint32_t sequence_=0;
    std::uint64_t now_=0;
    bool cloud_enabled_=true;
};
}
