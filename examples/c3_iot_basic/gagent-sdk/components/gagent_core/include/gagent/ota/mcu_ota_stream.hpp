#pragma once
#include "gagent/ota/mcu_ota_session.hpp"
#include "gagent/ota/ota_sink.hpp"
#include "gagent/ota/ota_verify.hpp"
#include <memory>
namespace gagent::ota {
// Single-worker HTTP -> UART ring. Only acknowledged bytes are discarded;
// Session/transport retain the active packet for retries. No Flash writes.
class Stream final : public Image,public ImageSink {
public:
    static constexpr std::size_t buffer_size=4096;
    // Same 16-bit packet-count bound as BinaryPlan, rounded down for HTTP headers.
    static constexpr std::uint32_t max_size=(65535u*(mcu::payload_limit-4)/4096)*4096;
    explicit Stream(Digest& digest) noexcept:verifier_(digest) {}
    std::size_t capacity() const noexcept override {return max_size;}
    bool busy() const noexcept override {return leased_;}
    bool begin(const Offer&) noexcept override;
    bool prepare_step() noexcept override {return state_==SinkState::receiving;}
    bool consumer_paced() const noexcept override {return true;}
    bool accepts(std::size_t n) const noexcept override {return n<=buffer_size-(written_-consumed_);}
    bool append(protocol::Bytes) noexcept override;
    bool seal() noexcept override;
    bool verify_step() noexcept override;
    bool cancel() noexcept override;
    Reason failure_reason() const noexcept override {return reason_;}
    SinkState sink_state() const noexcept override {return state_;}
    bool acquire() noexcept override;
    void release() noexcept override {
        leased_=false;
        // Session can release on rejection/timeout before Application cancels
        // HTTP. Invalidate first so a queued body event cannot use a freed ring.
        if(state_!=SinkState::ready)(void)fail();
        else bytes_.reset();
    }
    Offer offer() const noexcept override {return healthy()?offer_:Offer{};}
    bool healthy() const noexcept override {return bytes_ && (state_==SinkState::receiving || state_==SinkState::verifying || state_==SinkState::ready);}
    bool read(std::uint32_t offset,std::uint8_t* out,std::size_t n) noexcept override {return poll_read(offset,out,n)==ReadState::ready;}
    ReadState poll_read(std::uint32_t,std::uint8_t*,std::size_t) noexcept override;
    void consumed(std::uint32_t) noexcept override;
private:
    Reason reason_=Reason::unknown;
    bool fail(Reason reason=Reason::unknown) noexcept {reason_=reason;verifier_.cancel();state_=SinkState::failed;bytes_.reset();return false;}
    Verifier verifier_;Offer offer_{};
    // Allocate only when an opted-in download begins; release on completion/error.
    std::unique_ptr<std::array<std::uint8_t,buffer_size>> bytes_;
    std::uint32_t written_=0,consumed_=0;
    bool leased_=false;
    SinkState state_=SinkState::empty;
};
}
