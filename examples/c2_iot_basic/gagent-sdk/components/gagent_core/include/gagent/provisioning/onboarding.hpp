#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/network/wifi.hpp"
#include "gagent/protocol/protocol.hpp"
#include "gagent/cloud/identity.hpp"
namespace gagent::onboarding {
constexpr std::size_t payload_limit=320;
enum class Mode : std::uint8_t { ble=0, softap=1, airlink=2 };
enum class ModeResult { accepted, unavailable, unsupported };
// Request acceptance, not proof that the radio or cloud is already ready. Same Worker as Service.
class ModeControl {
public:
    virtual ~ModeControl()=default;
    virtual ModeResult request(Mode,std::uint64_t now) noexcept=0;
};
struct TemporaryResult {protocol::Status status;std::uint8_t code=0;bool acknowledge=false;};
class TemporaryControl {
public:
    virtual ~TemporaryControl()=default;
    virtual TemporaryResult temporary(const protocol::Context&,const protocol::Frame&,std::uint64_t now) noexcept=0;
};
struct Request {
    wifi::Credential credential{};
    std::uint32_t timestamp=0;
    bool registration_sn_set=false;
    int region=-1;
    bool timezone_set=false,location_set=false;
    std::int32_t timezone=0;
    unsigned location=0;
    std::array<char,60> domain{};
    std::size_t domain_size=0;
    unsigned config_flags=0x14,clear_flags=0;
};
protocol::Status parse(const protocol::Frame&,Request&) noexcept;
// MCU 0x09 payload, type 0x04: length-prefixed SSID/password and optional BSSID/timezone/domain.
protocol::Status parse_mcu(protocol::Bytes,Request&) noexcept;
// AirLink carries the same legacy password metadata; callback arrays must already be copied/bounded.
protocol::Status parse_airlink(const wifi::Credential&,Request&) noexcept;
using Random=identity::Random;
struct Policy {
    // Board supplies its legacy default CN endpoint; regions 1/2 match the old built-ins.
    std::array<std::string_view,3> domains{};
    std::uint16_t port=80;
    std::array<std::uint8_t,8> hardware{},software{};
    // Optional provider receipt, saved atomically with Wi-Fi credentials.
    std::string_view confirmation_key;
    bool (*confirmation)(const storage::Snapshot&,const Request&,char*,std::size_t) noexcept=nullptr;
};
struct Result {protocol::Status status;storage::WriteResult persistence;bool committed=false;bool replayed=false;protocol::Frame response{};bool has_response=false;};
// Shared by LAN and BLE, with one explicitly opened onboarding window. Long-lived, one worker.
class Service {
public:
    Service(runtime::Boot& boot,const Policy& policy,Random random) noexcept : boot_(boot),policy_(policy),random_(random) {}
    // Startup owner only; views refer to application-owned constants/buffers.
    bool policy(const Policy& value) noexcept {if(open_ || accepted_valid_)return false;policy_=value;return true;}
    void modes(ModeControl& control) noexcept {modes_=&control;}
    ModeResult request_mode(Mode mode,std::uint64_t now) noexcept;
    // Remains open until credentials are committed or the lifecycle explicitly closes it.
    bool open(std::uint64_t now,Mode mode=Mode::softap) noexcept;
    Mode mode() const noexcept {return mode_;}
    void close() noexcept;
    // Trusted lifecycle changes invalidate both replay state and old channel authorization.
    void invalidate() noexcept {close();++generation_;}
    bool is_open(std::uint64_t now) const noexcept {return open_ && now>=opened_at_;}
    Result receive(const protocol::Context&,const protocol::Frame&,std::uint64_t now) noexcept;
    // Trusted physical MCU entry: no LAN/BLE window bypass is exposed through their wire handlers.
    Result receive_mcu(protocol::Bytes,std::uint8_t sequence,std::uint64_t now) noexcept;
    // Trusted radio result, scoped to the exact window that started capture. Never callable through a wire command.
    Result receive_airlink(const wifi::Credential&,std::uint64_t window,std::uint64_t now) noexcept;
    std::uint64_t window_revision() const noexcept {return window_revision_;}
    std::uint64_t completion_revision() const noexcept {return completion_revision_;}
    std::uint64_t generation() const noexcept {return generation_;}
    // V2 progress uses the same layout on every channel; payload is borrowed until next call.
    protocol::Frame progress(std::uint8_t status,std::string_view ssid={}) noexcept;
private:
    friend class Trial;
    storage::WriteResult persist(const Request&,std::string_view configured_ssid={}) noexcept;
    runtime::Boot& boot_;Policy policy_;Random random_;
    ModeControl* modes_=nullptr;
    Request request_{};
    std::array<char,192> ssid_{},password_{},configured_ssid_{};
    std::array<char,13> bssid_{};
    identity::Passcode passcode_{};
    std::array<runtime::Change,20> changes_{};
    std::array<std::uint8_t,50> reply_{};
    std::array<std::uint8_t,payload_limit> accepted_{};
    std::size_t accepted_size_=0;
    protocol::Context accepted_from_{};
    std::uint16_t accepted_command_=0;
    std::uint64_t opened_at_=0,accepted_at_=0;
    std::uint64_t generation_=0,window_revision_=0,completion_revision_=0;
    enum class Source { local, mcu, airlink };
    Source accepted_source_=Source::local;
    std::uint64_t accepted_window_=0;
    Mode mode_=Mode::softap;
    bool open_=false,accepted_valid_=false;
};
// One pending RAM-only route change. The wireless owner calls complete only after
// the matching station attempt obtained an IP. This class never controls a radio.
class Trial {
public:
    enum class State {idle,pending,committed,expired,cancelled,failed};
    explicit Trial(Service& service) noexcept:service_(service) {}
    protocol::Status start(const protocol::Context&,const protocol::Frame&,std::uint64_t now) noexcept;
    State poll(std::uint64_t now) noexcept;
    storage::WriteResult complete(std::uint64_t now) noexcept;
    storage::WriteResult complete(std::uint64_t now,const wifi::Credential& connected) noexcept;
    void cancel() noexcept;
    const Request* request() const noexcept {return state_==State::pending?&request_:nullptr;}
    std::uint64_t deadline() const noexcept {return deadline_;}
private:
    void finish(State) noexcept;
    Service& service_;
    Request request_{};
    std::array<std::uint8_t,payload_limit> payload_{};
    std::size_t size_=0;
    protocol::Context source_{};
    State state_=State::idle;
    std::uint64_t now_=0,deadline_=0,generation_=0,window_=0;
};

}
