#pragma once
#include "gagent/cloud/mqtt_buffer.hpp"
#include <array>
#include <atomic>
namespace gagent::platform {
// Fixed slots, no allocation and no waiting. Callers exclusively own a token
// until release; queue publication provides the payload's happens-before edge.
template<class T,std::size_t N> class BufferPool {
    static_assert(N>0 && N<255);
public:
    mqtt::BufferToken acquire() noexcept {
        for(std::size_t i=0;i<N;++i)if(auto token=claim(i))return token;
        return {};
    }
    // Inspect candidates only while exclusively owned: a concurrent consumer
    // may still hold a slot, and payload metadata is deliberately non-atomic.
    // Lower rank wins. Unselected candidates are returned before this returns.
    template<class Rank> mqtt::BufferToken acquire_best(Rank rank) noexcept {
        mqtt::BufferToken best;std::size_t score=0;
        for(std::size_t i=0;i<N;++i)if(auto candidate=claim(i)){
            const auto value=rank(items_[i]);
            if(!best || value<score){if(best)release(best);best=candidate;score=value;}
            else release(candidate);
        }
        return best;
    }
    T* get(mqtt::BufferToken token) noexcept {
        return token && token.slot<N && generations_[token.slot].load(std::memory_order_acquire)==token.generation?&items_[token.slot]:nullptr;
    }
    bool release(mqtt::BufferToken token) noexcept {
        if(!token || token.slot>=N)return false;
        return generations_[token.slot].compare_exchange_strong(token.generation,token.generation+1,std::memory_order_acq_rel);
    }
    std::size_t used() const noexcept {
        std::size_t n=0;for(const auto& generation:generations_)n+=generation.load(std::memory_order_acquire)&1;return n;
    }
private:
    mqtt::BufferToken claim(std::size_t i) noexcept {
        auto generation=generations_[i].load(std::memory_order_acquire);
        if(!(generation&1) && generation!=UINT32_MAX-1 &&
           generations_[i].compare_exchange_strong(generation,generation+1,std::memory_order_acq_rel))return {generation+1,std::uint8_t(i)};
        return {};
    }
    std::array<T,N> items_{};
    std::array<std::atomic<std::uint32_t>,N> generations_{};
};
}
