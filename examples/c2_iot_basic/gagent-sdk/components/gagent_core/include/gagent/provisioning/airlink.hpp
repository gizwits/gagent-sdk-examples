#pragma once
#include "gagent/provisioning/onboarding.hpp"
namespace gagent::airlink {
enum class Kind { esptouch, airkiss };
struct Peer {std::array<std::uint8_t,4> address{};std::uint16_t port=0;};
struct ConfirmationInfo {Kind kind=Kind::esptouch;std::uint8_t token=0;std::array<std::uint8_t,6> mac{};std::array<std::uint8_t,4> phone{};};
struct Captured {
    wifi::Credential credential{};
    ConfirmationInfo confirmation{};
    std::uint64_t window=0;
};
enum class CaptureState {idle,active,received,stopping,failed,quarantined};
class Decoder {
public:
    virtual ~Decoder()=default;
    virtual bool start(std::uint64_t window,std::uint64_t now) noexcept=0;
    virtual CaptureState poll(std::uint64_t now) noexcept=0;
    virtual bool take(Captured&) noexcept=0;
    virtual void stop(std::uint64_t now) noexcept=0;
    virtual CaptureState state() const noexcept=0;
};
class Transport {
public:
    virtual ~Transport()=default;
    virtual bool open(const std::array<std::uint8_t,4>& local,std::uint16_t port) noexcept=0;
    // Nonblocking: 1 accepted, 0 would block, -1 failed. One datagram, never a partial write.
    virtual int send(Peer,protocol::Bytes) noexcept=0;
    // AirKiss's probe payload is ignored by the SDK, including an empty datagram.
    virtual int probe(Peer&) noexcept=0;
    virtual void close() noexcept=0;
};
enum class State { idle, waiting_for_network, waiting_for_probe, sending, sent, failed };
// Owns the confirmation socket on the same Worker as Wi-Fi and persistence. No ACK task outlives stop.
class Confirmation {
public:
    explicit Confirmation(Transport& io) noexcept :io_(io) {}
    ~Confirmation() {stop();}
    Confirmation(const Confirmation&)=delete;
    Confirmation& operator=(const Confirmation&)=delete;
    bool start(const onboarding::Result&,const wifi::Credential& saved,const ConfirmationInfo&,std::uint64_t now) noexcept;
    // selected is the credential actually used by the station manager, not whichever slot is currently preferred.
    void tick(std::uint64_t now,const wifi::Credential& selected,const wifi::Observation&) noexcept;
    void stop() noexcept;
    State state() const noexcept {return state_;}
    unsigned rounds() const noexcept {return rounds_;}
private:
    void fail() noexcept;
    Transport& io_;wifi::Credential expected_{};ConfirmationInfo info_{};
    std::array<std::uint8_t,4> address_{},destination_{};
    std::array<std::uint8_t,11> packet_{};
    std::uint64_t now_=0,earliest_=0,deadline_=0,probe_deadline_=0,next_=0;
    State state_=State::idle;unsigned rounds_=0;
    bool opened_=false,broadcast_=false;
};
}
