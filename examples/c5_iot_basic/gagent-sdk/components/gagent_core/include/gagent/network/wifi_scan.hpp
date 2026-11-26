#pragma once
#include "gagent/network/wifi.hpp"
namespace gagent::wifi {
// Bounded legacy slow scan: stop at the first nonempty list; retry empty scans
// for at most ten seconds. Results are released only after callback drain.
class ReconnectScan {
public:
    enum class State {idle,scanning,releasing,waiting,ready,failed,quarantined};
    enum class Match {exact_ssid,onboarding};
    explicit ReconnectScan(Survey& survey) noexcept:survey_(survey) {}
    bool start(const Credential&,std::uint64_t now,Match match=Match::exact_ssid) noexcept;
    void tick(std::uint64_t now) noexcept;
    void stop(std::uint64_t now) noexcept;
    State state() const noexcept {return state_;}
    bool result(Credential&) const noexcept;
private:
    void attempt(std::uint64_t now) noexcept;
    void release(std::uint64_t now) noexcept;
    Survey& survey_;
    Credential requested_{},selected_{};
    State state_=State::idle;
    Match match_=Match::exact_ssid;
    std::uint64_t now_=0,deadline_=0,drain_deadline_=0,retry_at_=0;
    bool selected_valid_=false,retry_=false,cancelled_=false;
};
}
