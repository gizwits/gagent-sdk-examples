#pragma once
#include "gagent/network/lan.hpp"
namespace gagent::lan {
enum class AnnouncementState {idle,sending,complete,failed};
// A single worker owns observation, sending, binding and durable completion.
class Announcements {
public:
    Announcements(Server& server,runtime::Boot& boot,onboarding::Service& onboarding,binding::Service& binding,bool cloud_registration_marker=false) noexcept
        :server_(server),boot_(boot),onboarding_(onboarding),binding_(binding),cloud_registration_marker_(cloud_registration_marker) {}
    void connected(std::uint64_t now) noexcept;
    void disconnected() noexcept {connected_=false;}
    void observe() noexcept;
    void tick(std::uint64_t now) noexcept;
    AnnouncementState state() const noexcept {return state_;}
private:
    bool configured() const noexcept;
    void finish() noexcept;
    Server& server_;runtime::Boot& boot_;onboarding::Service& onboarding_;binding::Service& binding_;
    std::uint64_t generation_=0,due_=0;
    unsigned startup_=20,onboarding_count_=0;
    bool connected_=false,seen_=false,binding_started_=false,send_failed_=false;
    bool cloud_registration_marker_=false;
    AnnouncementState state_=AnnouncementState::idle;
};
}
