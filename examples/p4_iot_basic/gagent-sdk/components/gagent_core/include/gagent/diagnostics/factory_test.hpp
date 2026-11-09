#pragma once
#include "gagent/network/wifi.hpp"
namespace gagent::factory {
// The radio adapter summarizes the legacy first 32 RSSI-ordered records. Scan-only
// matching uses the configured SSID prefix and the last matching record's RSSI.
struct ScanResult {bool any=false,target=false;std::int8_t rssi=0;};
// Consume SDK records in descending RSSI order without retaining an AP list.
// The caller supplies bounded SSID bytes without their trailing NUL.
class ScanSummary {
public:
    explicit ScanSummary(std::string_view target) noexcept:target_(target) {}
    void add(std::string_view ssid,std::int8_t rssi) noexcept;
    ScanResult result() const noexcept {return result_;}
private:
    std::string_view target_; // Borrowed only for the synchronous record enumeration.
    ScanResult result_{};
    unsigned count_=0;
};
using ScanState=wifi::ScanState;
class Scanner {
public:
    virtual ~Scanner()=default;
    // Same radio/Worker owner as wifi::Driver. Must reject active STA/AP/AirLink.
    // Copy the target. A new cycle is forbidden until idle proves callback drain.
    virtual bool start(std::string_view target,std::uint64_t now) noexcept=0;
    virtual ScanState poll(std::uint64_t now) noexcept=0;
    virtual bool result(ScanResult&) const noexcept=0;
    virtual void stop(std::uint64_t now) noexcept=0;
};
enum class Mode {connect,signal_only};
enum class SearchState {idle,scanning,releasing,backoff,ready,exhausted,quarantined};
// The shared search phase of factory mode. Owns no Wi-Fi instance, credentials,
// Flash or task. Wireless mode coordination consumes ready before connecting.
class Search {
public:
    explicit Search(Scanner& scanner) noexcept:scanner_(scanner) {}
    bool start(Mode,std::string_view target,std::uint64_t now) noexcept;
    void tick(std::uint64_t now) noexcept;
    void stop(std::uint64_t now) noexcept;
    SearchState state() const noexcept {return state_;}
    bool safe_to_release() const noexcept {return state_==SearchState::idle || state_==SearchState::ready || state_==SearchState::exhausted;}
    bool result(ScanResult&) const noexcept;
private:
    void attempt() noexcept;
    void release() noexcept;
    Scanner& scanner_;
    std::array<char,33> target_{};
    ScanResult result_{};
    SearchState state_=SearchState::idle;
    Mode mode_=Mode::connect;
    std::uint64_t now_=0,deadline_=0,scan_deadline_=0,release_deadline_=0,next_=0;
    unsigned attempts_=0;
    bool accepted_=false,cancelled_=false;
};
}
