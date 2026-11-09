#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
namespace gagent::protocol {
struct Bytes { const std::uint8_t* data = nullptr; std::size_t size = 0; };
enum class Status { ok, need_more, malformed, unsupported, too_large, unauthorized, wrong_device, handler_error };
// Internal metadata, never a V3 wire flag. MCU JSON includes its reserved action byte.
enum class PayloadFormat : std::uint8_t { binary, mcu_json };
struct Frame { std::uint8_t flags = 0; std::uint16_t command = 0; Bytes payload; PayloadFormat format=PayloadFormat::binary; };
struct Parsed { Status status; std::size_t consumed; Frame frame; };
// One V3 codec for LAN, MQTT application payload and GAgent BLE GATT, including shared onboarding frames.
// Returned views borrow the input; trailing concatenated frames are reported through consumed.
Parsed decode(Bytes, std::size_t max_payload = 4096) noexcept;
// Legacy ser2cli_res notices: BE32 version 3 + BE16 command + payload (no varint/flags).
Parsed decode_server_notice(Bytes,std::size_t max_payload=4096) noexcept;
Status encode(const Frame&, std::uint8_t* output, std::size_t capacity, std::size_t& written) noexcept;
enum class Channel { lan, mqtt, ble };
struct Context { Channel channel; std::uint32_t session; bool authorized; std::string_view device_id; };
struct Control { Bytes payload; std::uint32_t sequence; bool has_sequence; std::string_view device_id; PayloadFormat format=PayloadFormat::binary; };
class Endpoint {
public:
    virtual ~Endpoint() = default;
    virtual Status control(const Context&, const Control&) noexcept = 0;
};
class Reply {
public:
    virtual ~Reply() = default;
    // Frame/payload are borrowed for this call; async transports must copy.
    virtual Status send(const Context&, const Frame&) noexcept = 0;
};
// Central business handling; channel adapters only manage connections and authenticated context.
class Dispatcher {
public:
    Dispatcher(Endpoint& endpoint, Reply& reply) noexcept : endpoint_(endpoint), reply_(reply) {}
    Status dispatch(const Context&, const Frame&) noexcept;
private:
    Endpoint& endpoint_;
    Reply& reply_;
};
class Consumer {
public:
    virtual ~Consumer() = default;
    virtual Status on_frame(const Frame&) noexcept = 0;
};
// Each connection owns one decoder and caller-owned buffer. No allocations or silent resync.
// Any parse/consumer error latches until reset; prevents interpreting payload as a new header.
class Stream {
public:
    Stream(std::uint8_t* buffer, std::size_t capacity) noexcept : buffer_(buffer), capacity_(capacity) {}
    Status feed(Bytes, Consumer&) noexcept;
    void reset() noexcept { used_ = 0; error_ = Status::ok; }
private:
    std::uint8_t* buffer_;
    std::size_t capacity_;
    std::size_t used_ = 0;
    Status error_ = Status::ok;
};
}
