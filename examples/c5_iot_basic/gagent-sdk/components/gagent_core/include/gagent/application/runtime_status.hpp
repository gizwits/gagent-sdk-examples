#pragma once
#include "gagent/application/status.hpp"
#include "gagent/application/runtime.hpp"
#include "gagent/cloud/mqtt.hpp"
#include "gagent/network/wifi.hpp"
namespace gagent::runtime {
// Derives live business state from its owners. No copied login flags, extra loop or Flash writes.
// These objects and this service share the existing Worker lifetime; no SDK callback calls read().
class Status final : public status::Source {
public:
    Status(const Engine& engine,const wifi::Driver& wifi,const lan::Server& lan,const mqtt::Service& mqtt,
           const onboarding::Service& onboarding,const binding::Service& binding) noexcept
        : engine_(engine),wifi_(wifi),lan_(lan),mqtt_(mqtt),onboarding_(onboarding),binding_(binding) {}
    void ota(const status::OtaActivity& source) noexcept {ota_=&source;}
    void bluetooth(const ble::Control& source) noexcept {bluetooth_=&source;}
    bool read(std::uint64_t now,status::State&) const noexcept override;
    bool ipv4(std::array<std::uint8_t,4>&) const noexcept override;
private:
    const status::OtaActivity* ota_=nullptr;
    const ble::Control* bluetooth_=nullptr;
    const Engine& engine_;
    const wifi::Driver& wifi_;
    const lan::Server& lan_;
    const mqtt::Service& mqtt_;
    const onboarding::Service& onboarding_;
    const binding::Service& binding_;
};
}
