#pragma once
#include <cstdint>
namespace gagent::runtime {
class Restart {
public:
    virtual ~Restart()=default;
    virtual void restart() noexcept=0;
};
// One serialized lifecycle owner. The caller supplies its complete output-drain barrier.
// No task, timer allocation or protocol policy; stop must cancel before releasing the port.
class RestartScheduler {
public:
    bool action(Restart& action) noexcept {if(pending_)return false;action_=&action;return true;}
    bool available() const noexcept {return action_!=nullptr;}
    bool request(std::uint64_t now) noexcept {
        if(!action_)return false;
        if(pending_)return true; // Preserve the first accepted deadline, including after firing.
        deadline_=now>UINT64_MAX-2000?UINT64_MAX:now+2000;
        pending_=true;fired_=false;return true;
    }
    void tick(std::uint64_t now,bool drained) noexcept {
        if(!pending_ || fired_ || !drained || now<deadline_)return;
        fired_=true;action_->restart(); // Latch before a host substitute can call back.
    }
    void cancel() noexcept {pending_=fired_=false;deadline_=0;}
    bool pending() const noexcept {return pending_;}
    bool fired() const noexcept {return fired_;}
private:
    Restart* action_=nullptr;
    std::uint64_t deadline_=0;
    bool pending_=false,fired_=false;
};
}
