#pragma once
#include "gagent/network/local.hpp"
#include "gagent/build_features.hpp"
namespace gagent::lan {
using Peer=local::Peer;
using Discovery=local::Discovery;
using Encrypt=local::Encrypt;
using Transport=local::Transport;
inline constexpr std::size_t client_limit=(build::lan || build::softap)?local::client_limit:0;
inline constexpr auto frame_limit=local::frame_limit,send_limit=local::send_limit;
// LAN selects its channel; all local authentication/protocol/session business is shared with BLE.
#if defined(GAGENT_COMPACT_2M) && defined(GAGENT_BLE_ONLY_PROVISIONING)
// No local network transport exists in this build. Keep the runtime lifecycle
// interface without instantiating local::Server's 4.9 KiB protocol workspace.
class Server final : public device::Route {
public:
    Server(Transport&,device::Endpoint&,Encrypt=nullptr) noexcept {}
    bool configure(const storage::Snapshot&,const Discovery&) noexcept {configured_=true;return true;}
    bool refresh(const storage::Snapshot& s,const Discovery& d) noexcept {return configure(s,d);}
    bool configured() const noexcept {return configured_;}
    void stop() noexcept {configured_=false;}
    void router(device::Router&) noexcept {}
    void onboarding(onboarding::Service&) noexcept {}
    void resets(local::ResetControl&) noexcept {}
    void access_points(wifi::Driver&) noexcept {}
    void temporary(onboarding::TemporaryControl&) noexcept {}
    void binding(binding::Service&) noexcept {}
    bool onboarding_open(std::uint64_t) const noexcept {return false;}
    std::uint32_t accept(std::uint64_t) noexcept {return 0;}
    bool pending_output() const noexcept {return false;}
    std::size_t authenticated_clients(std::uint64_t) const noexcept {return 0;}
    void advance(std::uint64_t) noexcept {}
    void tick(std::uint64_t) noexcept {}
    void disconnect(std::uint32_t) noexcept {}
    protocol::Status receive(std::uint32_t,protocol::Bytes) noexcept {return protocol::Status::unsupported;}
    protocol::Status receive_datagram(Peer,protocol::Bytes) noexcept {return protocol::Status::unsupported;}
    protocol::Status announce(std::uint16_t) noexcept {return protocol::Status::unsupported;}
    protocol::Status send(const protocol::Context&,const protocol::Frame&) noexcept override {return protocol::Status::unsupported;}
    device::Broadcast broadcast(const protocol::Frame&) noexcept override {return {};}
    void failed(std::uint32_t) noexcept override {}
private:
    bool configured_=false;
};
#else
class Server final : public local::Server {
public:
    Server(Transport&,device::Endpoint&,Encrypt=nullptr) noexcept;
private:
    std::array<Session,client_limit> sessions_{};
};
#endif
}
