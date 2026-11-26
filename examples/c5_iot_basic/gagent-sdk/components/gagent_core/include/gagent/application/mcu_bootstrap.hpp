#pragma once
#include "gagent/application/runtime.hpp"
#include "gagent/application/mcu_product.hpp"
namespace gagent::runtime {
// A phase of the existing Worker. No private UART, decoder, retries or task.
class McuBootstrap {
public:
    // Workspace outlives this object. Released on activation, cancellation or
    // terminal failure; Application may then acquire it for its entire run.
    McuBootstrap(Boot& boot,device::Mcu& mcu,const mcu::ProductDefaults& defaults,identity::Random random,mcu::ProductWorkspace& workspace) noexcept
        : boot_(boot),mcu_(mcu),defaults_(defaults),random_(random),workspace_(workspace) {}
    McuBootstrap(const McuBootstrap&)=delete;
    McuBootstrap& operator=(const McuBootstrap&)=delete;
    ~McuBootstrap() {workspace_.release(this);}
    bool start(Engine&,std::uint64_t now,bool allow_migration=false) noexcept;
    void tick(Engine&,std::uint64_t now) noexcept;
    bool activate(Engine&,std::uint64_t now) noexcept;
    void cancel(Engine&) noexcept;
    bool recovering() const noexcept {return recovering_;}
    StartupState last_failure() const noexcept {return failure_;}
    bool cached() const noexcept {return cached_;}
    StartupState state() const noexcept {return state_;}
    // Startup-only observation; nullptr after ownership is handed off. Never
    // retain this pointer or its views across activate/cancel/restart/destruction.
    const mcu::Product* product() const noexcept {return workspace_.get(this);}
    Boot& boot() const noexcept {return boot_;}
    device::Mcu& mcu() const noexcept {return mcu_;}
private:
    void handshake_failed(Engine&,StartupState reason=StartupState::handshake_failure) noexcept;
    void invalid_product(Engine&) noexcept;
    // Twenty-one rounds, each retaining the previous five-second scheduling budget.
    static constexpr std::uint64_t handshake_budget_ms=device::Mcu::handshake_attempt_limit*5000;
    Boot& boot_;device::Mcu& mcu_;mcu::ProductDefaults defaults_;identity::Random random_;
    mcu::ProductWorkspace& workspace_;
    std::uint64_t now_=0,deadline_=0;
    bool cached_=false,recovering_=false,waiting_product_=false;
    StartupState failure_=StartupState::handshake_failure;
    StartupState state_=StartupState::not_requested;
};
}
