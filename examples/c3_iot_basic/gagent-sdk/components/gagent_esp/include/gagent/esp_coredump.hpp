#pragma once
#include "gagent/coredump_upload.hpp"
namespace gagent::platform {
// The Worker calls tick after business services. DNS runs asynchronously in
// lwIP's TCPIP thread; its independent context survives upload cancellation.
class EspCoredump {
public:
    void tick(std::uint64_t now,bool eligible,const diagnostics::Config&,bool flash_idle=true) noexcept;
private:
    struct State;
    State* state_=nullptr;
    diagnostics::Schedule schedule_;
    bool checked_guard_=false;
    int closing_fd_=-1;
    unsigned closing_tries_=0;
    std::uint64_t closing_due_=0;
};
}
