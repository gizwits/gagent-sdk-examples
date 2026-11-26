#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
namespace gagent::platform {
// Single producer/single consumer. reset requires both sides to be quiescent.
// N retains the original ring extent: usable capacity is N-1. Only usable slots
// are stored; the second index lap distinguishes full from empty without a spare T.
template<class T,std::size_t N> class Mailbox {
    static_assert(N>1 && N-1<=std::numeric_limits<std::size_t>::max()/2);
    static constexpr std::size_t capacity=N-1,period=2*capacity;
    static constexpr std::size_t next(std::size_t index) noexcept {return index+1==period ? 0 : index+1;}
    static constexpr std::size_t slot(std::size_t index) noexcept {return index<capacity ? index : index-capacity;}
    static constexpr std::size_t opposite(std::size_t index) noexcept {return index<capacity ? index+capacity : index-capacity;}
public:
    bool push(const T& item) noexcept {
        const auto tail=tail_.load(std::memory_order_relaxed);
        if(tail==opposite(head_.load(std::memory_order_acquire)))return false;
        items_[slot(tail)]=item;tail_.store(next(tail),std::memory_order_release);return true;
    }
    bool pop(T& item) noexcept {
        const auto head=head_.load(std::memory_order_relaxed);
        if(head==tail_.load(std::memory_order_acquire))return false;
        item=items_[slot(head)];head_.store(next(head),std::memory_order_release);return true;
    }
    bool full() const noexcept {return tail_.load(std::memory_order_relaxed)==opposite(head_.load(std::memory_order_acquire));}
    void reset() noexcept {head_.store(0);tail_.store(0);}
private:
    std::array<T,capacity> items_{};
    std::atomic<std::size_t> head_{0},tail_{0};
};
}
