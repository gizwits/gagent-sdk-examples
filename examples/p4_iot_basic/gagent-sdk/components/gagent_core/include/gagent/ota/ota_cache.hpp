#pragma once
#include "gagent/ota/mcu_ota_session.hpp"
#include "gagent/ota/ota_verify.hpp"
#include "gagent/ota/ota_sink.hpp"
namespace gagent::ota {
class CacheStore : public Reader {
public:
    virtual ~CacheStore()=default;
    virtual std::size_t capacity() const noexcept=0;
    virtual bool erase(std::size_t offset,std::size_t length) noexcept=0;
    virtual bool write(std::size_t offset,protocol::Bytes) noexcept=0;
    virtual bool read(std::size_t offset,std::uint8_t*,std::size_t) noexcept=0;
};
enum class CacheState {empty,erasing,receiving,verifying,ready,failed};
// One worker owns download, verification and image leases. No persistent validity marker.
class Cache final : public Image,public ImageSink {
public:
    Cache(CacheStore& store,Digest& digest,bool enabled) noexcept : store_(store),verifier_(digest),enabled_(enabled) {}
    Cache(const Cache&)=delete;
    Cache& operator=(const Cache&)=delete;
    bool begin(const Offer&) noexcept override;
    // One erase sector per worker step, before accepting download bytes.
    bool erase_step() noexcept;
    bool append(protocol::Bytes) noexcept override;
    bool seal() noexcept override;
    // Confirms the MD5 already accumulated while the bytes were accepted.
    bool verify_step() noexcept override;
    bool cancel() noexcept override;
    bool acquire() noexcept override;
    void release() noexcept override {leased_=false;}
    Offer offer() const noexcept override {return state_==CacheState::ready?offer_:Offer{};}
    bool read(std::uint32_t offset,std::uint8_t*,std::size_t) noexcept override;
    std::size_t capacity() const noexcept override {return enabled_?store_.capacity():0;}
    CacheState state() const noexcept {return state_;}
    bool leased() const noexcept {return leased_;}
    bool busy() const noexcept override {return leased_;}
    bool prepare_step() noexcept override {return erase_step();}
    SinkState sink_state() const noexcept override {
        switch(state_){
        case CacheState::empty:return SinkState::empty;
        case CacheState::erasing:return SinkState::preparing;
        case CacheState::receiving:return SinkState::receiving;
        case CacheState::verifying:return SinkState::verifying;
        case CacheState::ready:return SinkState::ready;
        case CacheState::failed:return SinkState::failed;
        }
        return SinkState::failed;
    }
private:
    bool fail() noexcept {verifier_.cancel();state_=CacheState::failed;return false;}
    CacheStore& store_;Verifier verifier_;
    Offer offer_{};std::size_t written_=0,erase_size_=0,erased_=0;
    bool enabled_=false,leased_=false;
    CacheState state_=CacheState::empty;
};
}
