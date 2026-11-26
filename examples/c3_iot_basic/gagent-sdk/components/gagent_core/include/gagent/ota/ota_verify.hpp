#pragma once
#include "gagent/ota/mcu_ota.hpp"
namespace gagent::ota {
class Reader {
public:
    virtual ~Reader()=default;
    virtual bool read(std::size_t offset,std::uint8_t*,std::size_t) noexcept=0;
};
class Digest {
public:
    virtual ~Digest()=default;
    virtual bool begin() noexcept=0;
    virtual bool update(protocol::Bytes) noexcept=0;
    virtual bool finish(std::array<std::uint8_t,16>&) noexcept=0;
};
enum class VerifyState {idle,working,ready,failed};
// MD5 of the bytes as they are accepted, matching the legacy downloader.
// The digest is not recomputed by reading the stored image back.
class Verifier {
public:
    explicit Verifier(Digest& digest) noexcept:digest_(digest) {}
    bool begin(const Offer&) noexcept;
    bool update(protocol::Bytes) noexcept;
    bool finish() noexcept;
    void cancel() noexcept {state_=VerifyState::idle;hashed_=0;}
    const std::array<std::uint8_t,16>& digest() const noexcept {return actual_;}
    VerifyState state() const noexcept {return state_;}
private:
    bool fail() noexcept {state_=VerifyState::failed;return false;}
    Digest& digest_;Offer offer_{};
    std::array<std::uint8_t,16> actual_{};std::uint32_t crc_=0xffffffffu;
    std::size_t hashed_=0;VerifyState state_=VerifyState::idle;
};
}
