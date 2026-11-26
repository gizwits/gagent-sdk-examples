#pragma once
#include "gagent/protocol/protocol.hpp"

namespace gagent::platform {
// Synchronous forwarding into the ordinary MCU receive stream. Borrowed bytes
// expire on return; false means the runtime could not accept them.
class SerialInput {
public:
    virtual ~SerialInput()=default;
    virtual bool forward(protocol::Bytes) noexcept=0;
};

// Optional board-owned UART demultiplexer. The object must outlive EspApplication.
// All calls run on the UART owner (reset also runs before Worker startup).
// No UART calls, blocking work, or calls back into Application are allowed here.
class SerialExtension {
public:
    virtual ~SerialExtension()=default;
    virtual void reset() noexcept=0;
    // Called every poll, including with empty input to expire partial requests.
    // Forward non-extension bytes unchanged and in order. Bound buffered state;
    // do not claim matching bytes inside an ordinary MCU frame.
    virtual bool receive(protocol::Bytes,std::uint64_t now,SerialInput&) noexcept=0;
    // Return one complete raw reply, or {}. The view remains valid until sent().
    // EspIo sends it after already queued ordinary output, on the same Worker.
    // At most one reply per poll; size must not exceed mcu::wire_limit.
    virtual protocol::Bytes reply() const noexcept=0;
    virtual void sent() noexcept=0;
};
}
