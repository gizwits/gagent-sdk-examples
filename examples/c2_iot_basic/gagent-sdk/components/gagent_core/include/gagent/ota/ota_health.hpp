#pragma once
#include <cstdint>
namespace gagent::ota {
enum class BootImageState {unknown,pending,valid,untracked};
class BootConfirmation {
public:
    virtual ~BootConfirmation()=default;
    virtual BootImageState image_state() noexcept=0;
    virtual bool confirm() noexcept=0;
    virtual bool rollback() noexcept=0;
};
enum class HealthState {unknown,waiting,confirmed,not_required,failed,rolled_back,rollback_failed};
// Continuous runtime health, measured by the existing worker's monotonic clock.
class Health {
public:
    explicit Health(BootConfirmation& boot) noexcept:boot_(boot) {}
    void tick(std::uint64_t now,bool healthy) noexcept;
    void interrupt() noexcept {timing_=false;}
    bool settled() const noexcept {return state_==HealthState::confirmed || state_==HealthState::not_required;}
    HealthState state() const noexcept {return state_;}
    bool pending() const noexcept {return state_==HealthState::unknown || state_==HealthState::waiting || state_==HealthState::failed;}
private:
    BootConfirmation& boot_;
    HealthState state_=HealthState::unknown;
    std::uint64_t since_=0,last_=0;
    bool timing_=false;
};
}
