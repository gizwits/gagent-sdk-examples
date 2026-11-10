#pragma once
#include "gagent/cloud/mqtt.hpp"
#include "gagent/network/wifi.hpp"
namespace gagent::c0 {
// Worker-owned channel telemetry. Provider capability gates all wire traffic.
class Service {
public:
    Service(runtime::Boot& boot,mqtt::Service& mqtt) noexcept : boot_(boot),mqtt_(mqtt) {}
    void observe(const wifi::Observation&,const wifi::Credential*,const std::array<std::uint8_t,6>& mac) noexcept;
    void tick(std::uint64_t now) noexcept;
    protocol::Status receive(const Request&,std::uint64_t now) noexcept;
    void reset() noexcept {valid_=sent_=false;epoch_=0;retry_at_=0;}
private:
    Sample current_{},last_{};
    std::uint32_t interval() const noexcept;
    runtime::Boot& boot_;mqtt::Service& mqtt_;
    std::uint64_t full_at_=0,retry_at_=0,connected_at_=0;
    std::uint32_t epoch_=0;
    bool valid_=false,sent_=false;
};
}
