#pragma once
#include "gagent/diagnostics/runtime_logs.hpp"
namespace gagent::platform {
// No buffer, task, timer or log hook exists before a valid enable command.
class EspRuntimeLogs final : public diagnostics::RuntimeLogs {
public:
    bool enable(const diagnostics::LogIdentity&) noexcept override;
    void stop() noexcept override;
    // Runtime owner updates upload eligibility without blocking on network I/O.
    void eligible(bool) noexcept;
    bool busy() const noexcept;
private:
    struct State;
    static void worker(void*) noexcept;
    State* state_=nullptr; // All access is protected by the module's critical section.
#ifdef GAGENT_C2_MEMORY_PROFILE
    void release() noexcept;
    // A failed nonblocking close is quarantined instead of leaking a new socket
    // on each retry. Owner keeps retrying cleanup, never blocking shutdown.
    int closing_fd_=-1;
#endif
};
}
