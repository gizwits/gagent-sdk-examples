#pragma once
#include "gagent/protocol/mcu.hpp"
#include "gagent/storage/storage.hpp"
namespace gagent::mcu::baud {
constexpr std::uint8_t request=0x73,response=0x74,version=1;
constexpr std::size_t payload_size=16;
constexpr std::uint32_t guard_ms=50,phase_ms=2000,handshake_ms=5000;
constexpr bool supported(std::uint32_t value) noexcept {
    return value==9600 || value==19200 || value==115200 || value==460800;
}
enum class Op : std::uint8_t {probe=1,prepare=2,ready=3,start=4,verify=5,confirm=6,complete=7,query=8,link_ready=9};
enum class Result : std::uint8_t {ok=0,invalid=1,busy=2,unsupported=3,not_saved=4,stale=5,timeout=6};
// Exactly 16 bytes: version, op, BE32 session, BE32 token, BE32 baud, result, detail.
// Probe request detail: 0=discover, 1=health. Probe response detail: capability bit 0.
// Prepare/start detail: fixed 50 ms guard. Other messages require detail=0.
struct Message {
    Op op=Op::probe;
    std::uint32_t session=0,token=0,rate=0;
    Result result=Result::ok;
    std::uint8_t detail=0;
};
bool decode(protocol::Bytes,Message&) noexcept;
std::array<std::uint8_t,payload_size> encode(const Message&) noexcept;
// Canonical handshake extensions only. 0=omitted; UINT32_MAX=invalid.
std::uint32_t desired(protocol::Bytes) noexcept;
// Record version and configured default are bound to the verified V2 snapshot.
std::uint32_t restored(const storage::Snapshot*,std::uint32_t default_rate) noexcept;
}
