#pragma once
#include "gagent/network/mdns.hpp"
namespace gagent::platform {
// Sole official mDNS instance, driven by the existing application worker.
class EspMdns final : public mdns::Publisher {
public:
    using Services=bool (*)() noexcept;
    // Optional product services share this instance and are restored after every reconnect.
    explicit EspMdns(Services services=nullptr) noexcept :services_(services) {}
    ~EspMdns() override {stop();}
    EspMdns(const EspMdns&)=delete;
    EspMdns& operator=(const EspMdns&)=delete;
    bool update(const mdns::Record&) noexcept override;
    void stop() noexcept override;
private:
    bool register_services() noexcept {return !services_ || services_();}
    Services services_=nullptr;
    mdns::Record current_{};
    bool active_=false;
    static EspMdns* owner_;
};
}
