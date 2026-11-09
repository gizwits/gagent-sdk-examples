#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>
namespace gagent::mqtt {
// Exclusive storage. Only its lease owner may grow it, before publishing any
// views. Capacity survives slot release and reconnect; destruction frees it.
template<std::size_t Limit> class PayloadBuffer {
    static_assert(Limit>=2048);
public:
    // Injectable realloc-compatible function for deterministic failure tests.
    using Reallocate=void* (*)(void*,std::size_t);
    PayloadBuffer() noexcept=default;
    ~PayloadBuffer(){std::free(data_);}
    PayloadBuffer(const PayloadBuffer&)=delete;
    PayloadBuffer& operator=(const PayloadBuffer&)=delete;
    PayloadBuffer(PayloadBuffer&& other) noexcept {swap(other);}
    PayloadBuffer& operator=(PayloadBuffer&& other) noexcept {
        if(this!=&other){PayloadBuffer old;old.swap(*this);swap(other);}return *this;
    }
    bool reserve(std::size_t requested,Reallocate reallocate=std::realloc) noexcept {
        if(requested>Limit)return false;
        if(requested<=capacity_)return true;
        const std::size_t next=requested<=512?512:requested<=1024?1024:requested<=2048?2048:Limit;
        // realloc failure leaves the old pointer, bytes and capacity intact.
        auto* grown=static_cast<std::uint8_t*>(reallocate(data_,next));
        if(!grown)return false;
        data_=grown;capacity_=next;return true;
    }
    // Worker-owned replay caches can release excess storage after completion.
    // A failed shrink preserves the valid reply and its original allocation.
    void trim(std::size_t used,Reallocate reallocate=std::realloc) noexcept {
        if(used>capacity_)return;
        if(!used){std::free(data_);data_=nullptr;capacity_=0;return;}
        const std::size_t next=used<=512?512:used<=1024?1024:used<=2048?2048:Limit;
        if(next>=capacity_)return;
        auto* smaller=static_cast<std::uint8_t*>(reallocate(data_,next));
        if(smaller){data_=smaller;capacity_=next;}
    }
    std::size_t size() const noexcept {return capacity_;} // Writable capacity, not message length.
    std::uint8_t* data() noexcept {return data_;}
    const std::uint8_t* data() const noexcept {return data_;}
    std::uint8_t& operator[](std::size_t i) noexcept {return data_[i];}
    const std::uint8_t& operator[](std::size_t i) const noexcept {return data_[i];}
private:
    void swap(PayloadBuffer& other) noexcept {std::swap(data_,other.data_);std::swap(capacity_,other.capacity_);}
    std::uint8_t* data_=nullptr;
    std::size_t capacity_=0;
};
}
