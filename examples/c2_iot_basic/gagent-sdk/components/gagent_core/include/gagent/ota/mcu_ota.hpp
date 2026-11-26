#pragma once
#include "gagent/protocol/mcu.hpp"
namespace gagent::ota {
enum class Format {bin,hex};
enum class Checksum {md5,crc32};
struct Offer {std::uint32_t size=0;std::array<std::uint8_t,16> md5{};Format format=Format::bin;Checksum checksum=Checksum::md5;std::uint32_t crc32=0;};
struct Ready {Format format=Format::bin;std::uint16_t piece_size=0;};
// Validates outgoing OTA request contracts before the shared UART transaction accepts them.
bool request_valid(std::uint8_t command,protocol::Bytes,std::uint16_t flags) noexcept;
// Payloads only; the existing MCU codec owns checksums, escaping and sequence numbers.
protocol::Status offer_payload(const Offer&,std::uint8_t*,std::size_t,std::size_t&) noexcept;
protocol::Status ready_payload(const Offer&,const mcu::Frame&,Ready&) noexcept;
struct BinaryPlan {
    std::uint32_t size=0;
    std::uint16_t piece_size=0,count=0;
    bool configure(std::uint32_t size,std::uint16_t requested_piece_size) noexcept;
    bool range(std::uint16_t index,std::uint32_t& offset,std::size_t& length) const noexcept;
};
protocol::Status binary_payload(const BinaryPlan&,std::uint16_t index,protocol::Bytes,std::uint8_t*,std::size_t,std::size_t&,std::uint16_t& flags) noexcept;
// One legacy HEX line, including CRLF except for an optional unterminated final line.
protocol::Status hex_payload(std::uint16_t index,bool last,protocol::Bytes,std::uint8_t*,std::size_t,std::size_t&,std::uint16_t& flags) noexcept;
}
