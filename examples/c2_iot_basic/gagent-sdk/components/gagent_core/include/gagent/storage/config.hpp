#pragma once
#include "gagent/storage/storage.hpp"
namespace gagent::config {
enum class Owner { product, identity, connection, preference, wifi, state, diagnostic, deferred };
struct Field { storage::FieldRule rule; Owner owner; };
storage::Schema schema() noexcept;
const Field* fields(std::size_t& count) noexcept;
enum class Reset { wifi, unbind, factory, factory_ble };
// Reset callbacks collect changes; they never build intermediate snapshots.
// Borrowed keys/values must survive until reset merges them: source views and
// static strings are safe; callback-local buffers are not.
class ResetChanges {
public:
    bool set(std::string_view key,std::string_view value) noexcept {
        for(std::size_t i=0;i<size_;++i)if(fields_[i].key==key){fields_[i].value=value;return true;}
        if(size_==fields_.size())return false;
        fields_[size_++]={key,value};return true;
    }
    const storage::FieldValue* data() const noexcept {return fields_.data();}
    std::size_t size() const noexcept {return size_;}
private:
    std::array<storage::FieldValue,64> fields_{};
    std::size_t size_=0;
};
using ResetTransform=storage::Status(*)(const storage::Snapshot&,ResetChanges&,Reset) noexcept;
// Shared transformation plus provider changes, committed as one transaction.
storage::Status reset(const storage::Snapshot&,storage::Snapshot&,Reset,const storage::Codec&,ResetTransform=nullptr) noexcept;
// Single serialized owner. Mutations only change the staged snapshot; save is explicit.
// No passcode generation, cloud deregistration or automatic writes on load.
class Service {
public:
    Service(storage::ConfigStore& store, storage::Md5 md5,ResetTransform reset=nullptr) noexcept : store_(store), codec_(md5),reset_(reset) {}
    storage::LoadResult load() noexcept;
    std::string_view get(std::string_view key) const noexcept;
    storage::Status set(std::string_view key, std::string_view value) noexcept;
    storage::Status reset(Reset) noexcept;
    storage::WriteResult save() noexcept;
    storage::WriteResult repair() noexcept;
    bool ready() const noexcept { return ready_; }
private:
    storage::ConfigStore& store_;
    storage::Codec codec_;
    storage::Snapshot current_{};
    storage::Snapshot work_{};
    bool ready_ = false;
    ResetTransform reset_=nullptr;
};
}
