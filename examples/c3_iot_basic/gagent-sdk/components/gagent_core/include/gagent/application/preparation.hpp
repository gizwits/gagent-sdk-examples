#pragma once
#include "gagent/application/runtime.hpp"
namespace gagent::runtime {
// Runs on the existing Worker after durable startup/MCU probing, before normal device activation.
class Preparation {
public:
    virtual ~Preparation()=default;
    virtual bool prepare(Engine&,lan::Discovery&) noexcept=0;
    // Called after radio/mode owners have started, before the first station attempt.
    virtual bool activated(std::uint64_t now) noexcept=0;
    virtual void advance(std::uint64_t) noexcept {}
    // Trial-boot deadline continues while startup fails or the normal worker is stopping.
    virtual void boot_tick(std::uint64_t) noexcept {}
    virtual bool boot_pending() const noexcept {return false;}
    virtual void stopped() noexcept {}
    // Worker remains alive until asynchronous cancellation has released owned resources.
    virtual bool drain_stop() noexcept {return true;}
};
}
