#pragma once
#include "gagent/protocol/protocol.hpp"
#include <array>
namespace gagent::beacon {
// Legacy MCU 0x64 payload. Result values are the one-byte 0x65 response contract.
enum class Result : std::uint8_t {ok=0,length=1,enable=2,unavailable=3};
struct Request {
    bool enabled=false;
    std::uint8_t address_type=0xff; // Values above 3 disable address-type filtering.
    std::array<std::uint8_t,6> address{}; // All zero disables address filtering.
    std::array<std::uint8_t,31> prefix{};
    std::uint8_t prefix_size=0;
};
enum class State {disabled,starting,enabled,stopping,failed};
class Control {
public:
    virtual ~Control()=default;
    virtual bool beacon_request(const Request&,std::uint64_t) noexcept=0;
    virtual State beacon_state() const noexcept=0;
};
// Owns the command bytes; failure never partially replaces the caller's request.
Result decode(protocol::Bytes,Request&) noexcept;
struct Report {
    std::uint8_t address_type=0;
    std::array<std::uint8_t,6> address{};
    std::array<std::uint8_t,31> data{};
    std::uint8_t size=0;
    std::int8_t rssi=0;
};
// Board-owned consumer, normally supplied by the MCU board. Called only on the
// application worker after filtering; never on the NimBLE host callback.
// The reference is valid for this call only. Copy it if work must be deferred.
class Receiver {
public:
    virtual ~Receiver()=default;
    virtual void observed(const Report&) noexcept=0;
};
// Common filtering on the application worker, independent of NimBLE or any transport.
// With any filter enabled, legacy deduplication compares the last accepted payload,
// even across advertisers. Unfiltered observations are delivered without deduplication.
class Filter {
public:
    bool configure(const Request&) noexcept;
    bool accept(const Report&) noexcept;
    void clear() noexcept {previous_={};previous_size_=0;have_previous_=false;}
private:
    Request request_{};
    std::array<std::uint8_t,31> previous_{};
    std::uint8_t previous_size_=0;
    bool have_previous_=false;
};
class Delivery {
public:
    explicit Delivery(Receiver& receiver) noexcept : receiver_(receiver) {}
    bool configure(const Request& request) noexcept {return filter_.configure(request);}
    void receive(const Report& report) noexcept {if(filter_.accept(report))receiver_.observed(report);}
private:
    Receiver& receiver_;
    Filter filter_;
};
}
