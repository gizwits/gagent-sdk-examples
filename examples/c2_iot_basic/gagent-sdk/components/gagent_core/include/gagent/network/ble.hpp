#pragma once
#include "gagent/network/local.hpp"
#include "gagent/network/beacon.hpp"
namespace gagent::runtime {class Engine;}
namespace gagent::ble {
inline constexpr std::size_t client_limit=1;
#ifdef GAGENT_C2_MEMORY_PROFILE
inline constexpr std::size_t send_limit=local::frame_limit;
#else
inline constexpr std::size_t send_limit=local::send_limit;
#endif
using Transport=local::Transport;
using Discovery=local::Discovery;
using Encrypt=local::Encrypt;
enum class State {disabled,starting,enabled,stopping,failed};
using ProductEncoder=bool(*)(const storage::Snapshot&,std::uint8_t*,std::size_t,std::size_t&) noexcept;
class Control {
public:
    virtual ~Control()=default;
    virtual bool set_enabled(bool,std::uint64_t now) noexcept=0;
    virtual State state() const noexcept=0;
    virtual bool enabled() const noexcept {return state()==State::enabled;}
    virtual bool connected(std::uint64_t) const noexcept {return false;}
};
class Radio : public Transport {
public:
    virtual void product_encoder(ProductEncoder) noexcept {}
    virtual bool begin(const storage::Snapshot&,std::string_view,const std::array<std::uint8_t,6>&,bool,bool=false) noexcept=0;
    virtual void poll(runtime::Engine&,std::uint64_t) noexcept=0;
    virtual void cancel() noexcept=0;
    virtual bool busy() const noexcept=0;
    virtual bool ready() const noexcept=0;
    virtual bool advertising() const noexcept=0;
    virtual void onboarding(bool) noexcept=0;
    virtual void control(bool) noexcept {}
    virtual bool control_active() const noexcept {return ready();}
    virtual bool beacon_capable() const noexcept {return false;}
    virtual bool scan(bool) noexcept {return false;}
    virtual beacon::State scan_state() const noexcept {return beacon::State::disabled;}
    virtual bool next_beacon(beacon::Report&) noexcept {return false;}
};
struct Policy {bool skip_login=false;}; // Trusted product policy; never enabled by a received packet.
// GATT moves bytes for a connection; it does not implement another login/dispatch/onboarding stack.
class Server final : public local::Server {
public:
    Server(Transport&,device::Endpoint&,Encrypt=nullptr,Policy={}) noexcept;
private:
    Session sessions_[client_limit]{};
    std::array<std::uint8_t,client_limit*local::frame_limit> input_{};
    std::array<std::uint8_t,client_limit*send_limit> output_{};
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    V5Workspace workspace_{};
#endif
};
}
