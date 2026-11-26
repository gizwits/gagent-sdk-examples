#pragma once
#include "gagent/protocol/protocol.hpp"
#include "gagent/cloud/identity.hpp"
#include "gagent/network/ble_protocol.hpp"
#include "gagent/storage/storage.hpp"
#include <array>
namespace gagent::ble::v5 {
using Hash=bool(*)(const std::uint8_t*,std::size_t,std::uint8_t*) noexcept;
using SetTime=void(*)(void*,std::uint32_t,std::int8_t,std::uint64_t) noexcept;
struct Crypto {
    identity::Random random=nullptr;
    bool (*exchange)(protocol::Bytes,std::uint8_t* public_key,std::uint8_t* shared_key,identity::Random) noexcept=nullptr;
    bool (*encrypt)(const std::uint8_t* key,const std::uint8_t* nonce,protocol::Bytes,std::uint8_t* cipher,std::uint8_t* tag) noexcept=nullptr;
    bool (*decrypt)(const std::uint8_t* key,const std::uint8_t* nonce,protocol::Bytes,const std::uint8_t* tag,std::uint8_t* plain) noexcept=nullptr;
    explicit operator bool() const noexcept {return random && exchange && encrypt && decrypt;}
};
constexpr std::size_t max_frame=255,max_payload=4016;
std::uint32_t product_id(std::string_view) noexcept;
bool key(std::string_view pk,std::string_view secret,const std::array<std::uint8_t,6>& mac,Hash,std::array<std::uint8_t,16>&,bool uppercase=true) noexcept;
// One complete ATT value per feed; strict contiguous ordering, no silent resync.
struct Assembly {
    std::size_t used=0;
    std::uint8_t id=0,command=0,total=0,next=0;
    void reset() noexcept {used=0;id=command=total=next=0;}
    protocol::Status feed(protocol::Bytes,std::uint8_t* buffer,std::size_t capacity,protocol::Frame&) noexcept;
};
protocol::Status encode(const protocol::Frame&,std::size_t att_capacity,std::uint8_t*,std::size_t,std::size_t&) noexcept;
// 0xABD4, never includes the product secret.
std::size_t information(const storage::Snapshot&,protocol::Bytes hardware,protocol::Bytes software,std::uint8_t*,std::size_t) noexcept;
}
