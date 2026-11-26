#pragma once
#include <cstddef>
#include <cstdint>
#include <utility>
namespace gagent::mqtt {
// Generations never wrap: an exhausted slot is retired. The connection epoch is
// carried by the message, independently of slot lifetime across reconnects.
struct BufferToken {
    std::uint32_t generation=0;
    std::uint8_t slot=255;
    explicit operator bool() const noexcept {return slot!=255 && (generation&1);}
    bool operator==(BufferToken b) const noexcept {return slot==b.slot && generation==b.generation;}
};
// Exclusive, scoped ownership. A transport must outlive all its leases.
template<class T> class BufferLease {
public:
    using Release=void (*)(void*,BufferToken) noexcept;
    BufferLease() noexcept=default;
    ~BufferLease(){reset();}
    BufferLease(const BufferLease&)=delete;
    BufferLease& operator=(const BufferLease&)=delete;
    BufferLease(BufferLease&& other) noexcept {swap(other);}
    BufferLease& operator=(BufferLease&& other) noexcept {if(this!=&other){reset();swap(other);}return *this;}
    explicit operator bool() const noexcept {return value_!=nullptr;}
    T* operator->() const noexcept {return value_;}
    T& operator*() const noexcept {return *value_;}
    void* owner() const noexcept {return owner_;}
    BufferToken token() const noexcept {return token_;}
    void reset() noexcept {
        auto release=release_;auto* owner=owner_;const auto token=token_;
        detach();if(release)release(owner,token);
    }
    // Transport-only ownership transfer. No buffer is copied or freed here.
    BufferToken detach() noexcept {
        const auto token=token_;value_=nullptr;owner_=nullptr;release_=nullptr;token_={};return token;
    }
    void attach(T& value,void* owner,BufferToken token,Release release) noexcept {
        reset();value_=&value;owner_=owner;token_=token;release_=release;
    }
private:
    void swap(BufferLease& other) noexcept {
        std::swap(value_,other.value_);std::swap(owner_,other.owner_);std::swap(token_,other.token_);std::swap(release_,other.release_);
    }
    T* value_=nullptr;void* owner_=nullptr;BufferToken token_{};Release release_=nullptr;
};
}
