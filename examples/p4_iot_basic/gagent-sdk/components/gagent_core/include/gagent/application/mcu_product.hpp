#pragma once
#include "gagent/protocol/mcu.hpp"
#include "gagent/application/startup.hpp"
#include "gagent/protocol/mcu_dialect.hpp"
namespace gagent::mcu {
struct ProductDefaults {
    std::string_view host;
    std::uint16_t port=0;
    std::array<std::uint8_t,6> mac{};
    const ProductPolicy* policy=nullptr;
};
// Owns the bounded handshake and all derived views; no allocation or borrowed UART buffer.
// This object must not be copied/moved: its configuration views point into its own buffers.
class Product {
public:
    Product()=default;
    Product(const Product&)=delete;
    Product& operator=(const Product&)=delete;
    bool parse(protocol::Bytes,const ProductDefaults&) noexcept;
    // Reconstruct the legacy fixed handshake from explicit, matching cached identity.
    protocol::Bytes restore(const storage::Snapshot&,const ProductDefaults&) noexcept;
    const runtime::StartupDefaults* configuration() const noexcept {return valid_?&configuration_:nullptr;}
    void clear() noexcept;
    protocol::Bytes wire() const noexcept {return valid_?protocol::Bytes{bytes_.data(),size_}:protocol::Bytes{};}
private:
    bool add(std::string_view,std::string_view,bool allow_empty=false) noexcept;
    std::array<std::uint8_t,payload_limit> bytes_{};
    std::array<char,17> attributes_{};
    std::array<char,6> bind_time_{};
    std::array<char,66> ap_name_{};
    std::array<runtime::Change,48> fields_{};
    runtime::StartupDefaults configuration_{};
    std::size_t count_=0,size_=0;
    bool valid_=false;
};
// Single runtime-owner workspace. Bootstrap and the activated application hold
// exclusive phases, including while waiting for I/O. This is not a cross-task
// lock: all callers run on the same Worker. Views expire when the owner releases.
class ProductWorkspace {
public:
    bool available(const void* owner) const noexcept {return owner && (!owner_ || owner_==owner);}
    Product* acquire(const void* owner) noexcept {
        if(!available(owner))return nullptr;
        owner_=owner;return &product_;
    }
    Product* get(const void* owner) noexcept {return owner && owner_==owner?&product_:nullptr;}
    const Product* get(const void* owner) const noexcept {return owner && owner_==owner?&product_:nullptr;}
    void release(const void* owner) noexcept {
        if(!get(owner))return;
        product_.clear();owner_=nullptr;
    }
private:
    Product product_;
    const void* owner_=nullptr;
};
}
