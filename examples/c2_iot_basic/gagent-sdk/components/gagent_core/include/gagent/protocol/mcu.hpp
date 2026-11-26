#pragma once
#include "gagent/protocol/protocol.hpp"
#include <array>
namespace gagent::mcu {
constexpr std::size_t payload_limit=2048;
constexpr std::size_t wire_limit=2*(payload_limit+7)+2;
struct Frame { std::uint8_t command=0, sequence=0; std::uint16_t flags=0; protocol::Bytes payload; protocol::PayloadFormat format=protocol::PayloadFormat::binary; };
protocol::Status encode(const Frame&, std::uint8_t*, std::size_t, std::size_t&) noexcept;
class Consumer {
public:
    virtual ~Consumer()=default;
    virtual void received(const Frame&) noexcept=0;
    // Complete, bounded frame only. No unverified payload crosses this boundary.
    virtual void checksum_failed(std::uint8_t command,std::uint8_t sequence) noexcept {(void)command;(void)sequence;}
};
// Exact legacy checksum-error suppression set; not a general command dispatcher.
bool checksum_error_suppressed(std::uint8_t command) noexcept;
// Escaped FF bytes, bounded length, checksum. Reset on malformed bytes; received is never called for bad frames.
class Decoder {
public:
    protocol::Status feed(protocol::Bytes, Consumer&) noexcept;
    void reset() noexcept { sync_=0;used_=0;escaped_=false;expected_=0; }
private:
    std::array<std::uint8_t,payload_limit+7> raw_{};
    std::size_t used_=0,expected_=0;
    unsigned sync_=0;
    bool escaped_=false;
};
}
