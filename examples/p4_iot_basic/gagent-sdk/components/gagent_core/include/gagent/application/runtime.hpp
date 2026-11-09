#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/network/lan.hpp"
#include "gagent/network/ble.hpp"
namespace gagent::runtime {
// Without LAN/UDP, handoff chunks are 256 bytes on C2, 512 on other chips.
// Full-frame assemblers retain the original protocol limits.
#ifdef GAGENT_C2_MEMORY_PROFILE
inline constexpr std::size_t event_bytes=(!build::lan && !build::softap)?256:1024;
#else
inline constexpr std::size_t event_bytes=(!build::lan && !build::softap)?512:1024;
#endif
enum class EventKind { tcp, udp, serial, disconnected, network_down, bluetooth, bluetooth_disconnected };
struct Event {
    EventKind kind=EventKind::network_down;
    std::uint32_t session=0;
    lan::Peer peer{};
    std::array<std::uint8_t,event_bytes> bytes{};
    std::size_t size=0;
};
// Single-owner event loop. Only the platform worker calls these methods.
// Other RTOS tasks must use their own synchronized handoff to that worker, not mutate Engine.
class Engine {
public:
    Engine(Boot& boot,lan::Server& lan,device::Endpoint& endpoint) noexcept : boot_(boot),lan_(lan),endpoint_(endpoint),router_(endpoint) {router_.bind(protocol::Channel::lan,lan_);lan_.router(router_);}
    bool start(const lan::Discovery&,std::uint64_t now,bool allow_migration=false) noexcept;
    // Device lifetime is independent of Wi-Fi/cloud identity. Requires a validated seed only.
    bool start_device(std::uint64_t now,bool allow_migration=false) noexcept;
    bool probe_mcu(device::Mcu&,std::uint64_t now,bool allow_migration=false) noexcept;
    bool promote_mcu(device::Mcu&,std::uint64_t now) noexcept;
    bool probing() const noexcept {return probing_;}
    bool start_network(const lan::Discovery&,std::uint64_t now) noexcept;
    void stop_network() noexcept;
    bool refresh_network(const lan::Discovery&) noexcept;
    bool onboarding(onboarding::Service& service) noexcept {if(active_)return false;lan_.onboarding(service);return true;}
    bool bluetooth(ble::Server&) noexcept;
    bool start_bluetooth(const lan::Discovery&,std::uint64_t now) noexcept;
    bool refresh_bluetooth(const lan::Discovery&) noexcept;
    void stop_bluetooth() noexcept;
    bool bluetooth_active() const noexcept {return bluetooth_active_;}
    std::uint32_t accept_bluetooth(std::uint64_t now) noexcept;
    bool pending_output() const noexcept {return lan_.pending_output();}
    bool pending_bluetooth_output() const noexcept {return bluetooth_ && bluetooth_->pending_output();}
    bool pending_local_output() const noexcept {return lan_.pending_output() || (bluetooth_ && bluetooth_->pending_output());}
    bool network_active() const noexcept {return network_active_;}
    std::uint32_t accept(std::uint64_t now) noexcept;
    bool post(EventKind,std::uint32_t,lan::Peer,protocol::Bytes={}) noexcept;
    void drain(std::uint64_t now) noexcept;
    // Platform Worker only: consume BLE TX completions without repeating the
    // endpoint/LAN timers or running a second full application turn.
    void flush_bluetooth(std::uint64_t now) noexcept {
        if(active_ && bluetooth_active_ && bluetooth_ && !draining_ && now>=now_)bluetooth_->tick(now);
    }
    void stop() noexcept;
    bool active() const noexcept {return active_;}
    Boot& boot() const noexcept {return boot_;}
    bool owns(const device::Endpoint& endpoint) const noexcept {return &endpoint==&endpoint_;}
    device::Router& router() noexcept {return router_;}
    std::size_t queued() const noexcept {return count_;}
private:
    ble::Server* bluetooth_=nullptr;
    bool bluetooth_active_=false;
    Boot& boot_;
    lan::Server& lan_;
    device::Endpoint& endpoint_;
    device::Router router_;
    std::array<Event,8> events_{};
    std::size_t head_=0,count_=0;
    std::uint64_t now_=0;
    BootState prepare_storage(bool allow_migration) noexcept;
    bool active_=false,network_active_=false,draining_=false,probing_=false;
};
}
