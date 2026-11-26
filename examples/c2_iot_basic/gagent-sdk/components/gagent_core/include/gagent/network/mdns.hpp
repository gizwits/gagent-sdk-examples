#pragma once
#include "gagent/storage/storage.hpp"
namespace gagent::mdns {
struct Record {
    std::array<char,64> name{};
    std::array<char,61> api{};
    std::array<char,6> port{};
    std::array<char,33> product_key{};
    bool operator==(const Record& other) const noexcept {return name==other.name && api==other.api && port==other.port && product_key==other.product_key;}
};
bool record(const storage::Snapshot&,std::string_view name,Record&) noexcept;
class Publisher {
public:
    virtual ~Publisher()=default;
    virtual bool update(const Record&) noexcept=0;
    virtual void stop() noexcept=0;
};
}
