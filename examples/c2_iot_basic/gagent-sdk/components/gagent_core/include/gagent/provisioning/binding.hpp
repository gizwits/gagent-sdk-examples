#pragma once
#include "gagent/provisioning/onboarding.hpp"
namespace gagent::device {class Router;}
namespace gagent::binding {
// One physical/board-authorized binding service shared by local channels.
class Service {
public:
    Service(runtime::Boot& boot,onboarding::Service& onboarding) noexcept : boot_(boot),onboarding_(onboarding) {}
    Service(const Service&)=delete;
    Service& operator=(const Service&)=delete;
    void reset_notifications(device::Router& router) noexcept {router_=&router;}
    bool set_reset_permission(bool) noexcept;
    bool reset_allowed() const noexcept;
    protocol::Status notify_permission(const protocol::Context&,protocol::Reply&) noexcept;
    bool open(std::uint64_t now,std::uint16_t seconds) noexcept;
    void close() noexcept {enabled_=false;response_.fill(0);}
    bool is_open(std::uint64_t now) const noexcept;
    // Trusted same-product lifecycle only. Preserve an existing finite authorization,
    // including elapsed teardown time; never convert unlimited/closed state into a window.
    bool finite_deadline(std::uint64_t now,std::uint64_t& deadline) const noexcept;
    bool resume_finite(std::uint64_t now,std::uint64_t deadline) noexcept;
    protocol::Status receive(const protocol::Context&,const protocol::Frame&,std::uint64_t now,protocol::Reply&) noexcept;
private:
    runtime::Boot& boot_;
    onboarding::Service& onboarding_;
    device::Router* router_=nullptr;
    std::array<std::uint8_t,34> response_{};
    std::uint64_t opened_at_=0,deadline_=0,generation_=0;
    bool enabled_=false,unlimited_=false;
};
}
