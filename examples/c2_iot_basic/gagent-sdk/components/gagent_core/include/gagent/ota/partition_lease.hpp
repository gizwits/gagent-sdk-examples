#pragma once
#include <atomic>
namespace gagent::ota {
// One lease covers both Wi-Fi image writes and voice-image staging/reads.
// Inline storage is shared across translation units and IDF components.
class PartitionLease {
public:
    bool acquire() noexcept {
        if (held_) return true;
        const void *empty = nullptr;
        held_ = owner_.compare_exchange_strong(empty, this);
        return held_;
    }
    void release() noexcept {
        if (held_) { owner_.store(nullptr); held_ = false; }
    }
    ~PartitionLease() { release(); }
    PartitionLease() = default;
    PartitionLease(const PartitionLease &) = delete;
    PartitionLease &operator=(const PartitionLease &) = delete;
private:
    inline static std::atomic<const void *> owner_{nullptr};
    bool held_ = false;
};
}
