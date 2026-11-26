#pragma once
#include "gagent/protocol/device.hpp"
#include <atomic>

namespace gagent::runtime {
// One voice RX producer, one application-worker consumer. No UART access from RX.
class VoiceReports final : private device::McuCompletion {
public:
    enum class Event { sent, accepted, rejected, unconfirmed };
    using Observer = void (*)(Event,std::uint32_t,int) noexcept;
    explicit VoiceReports(device::Mcu* mcu) noexcept : mcu_(mcu) {}
    void observer(Observer value) noexcept {observer_=value;} // Before tasks start.
    bool post(std::uint32_t id) noexcept {
        const auto generation=generation_.load(std::memory_order_acquire);
        if(!(generation&1))return false;
        const auto tail=tail_.load(std::memory_order_relaxed);
        const auto next=(tail+1)%items_.size();
        if(next==head_.load(std::memory_order_acquire))return false;
        items_[tail]={id,generation};
        tail_.store(next,std::memory_order_release);return true;
    }
    void tick(bool enabled) noexcept {
        enabled=enabled && mcu_ && mcu_->available();
        const auto generation=generation_.load(std::memory_order_relaxed);
        if(bool(generation&1)!=enabled)generation_.store(generation+1,std::memory_order_release);
        if(!enabled && pending_)mcu_->cancel(*this);
        // Drop events across a link stop/reset; never replay an uncertain command.
        for(std::size_t i=0;i<items_.size();++i){
            const auto head=head_.load(std::memory_order_relaxed);
            if(head==tail_.load(std::memory_order_acquire))return;
            const auto item=items_[head];
            if(enabled && item.generation==generation_.load(std::memory_order_relaxed)){
                if(pending_)return;
                const std::uint8_t bytes[]={std::uint8_t(item.id>>24),std::uint8_t(item.id>>16),std::uint8_t(item.id>>8),std::uint8_t(item.id)};
                if(!mcu_->request(device::McuRequest::voice_command,{bytes,4},*this))return;
                pending_=true;current_=item.id;
                if(observer_)observer_(Event::sent,current_,-1);
            }
            head_.store((head+1)%items_.size(),std::memory_order_release);
        }
    }
private:
    void completed(device::McuRequest,bool success,protocol::Bytes result) noexcept override {
        pending_=false;
        if(observer_)observer_(success?Event::accepted:result.size==1?Event::rejected:Event::unconfirmed,
                               current_,result.size==1?result.data[0]:-1);
    }
    struct Item {std::uint32_t id=0;std::uint32_t generation=0;};
    device::Mcu* mcu_;
    std::array<Item,17> items_{}; // 16 queued events plus one in flight.
    std::atomic<std::size_t> head_{0},tail_{0};
    std::atomic<std::uint32_t> generation_{0};
    bool pending_=false;std::uint32_t current_=0;Observer observer_=nullptr;
};
}
