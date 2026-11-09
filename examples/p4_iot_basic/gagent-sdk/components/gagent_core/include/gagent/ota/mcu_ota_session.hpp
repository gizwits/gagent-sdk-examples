#pragma once
#include "gagent/protocol/device.hpp"
#include "gagent/ota/mcu_ota.hpp"
namespace gagent::ota {
enum class ReadState {ready,pending,failed};
// The provider must outlive the active session. Streaming providers withhold the
// final read until the entire download has passed size and digest verification.
class Image {
public:
    virtual ~Image()=default;
    virtual bool acquire() noexcept {return true;}
    virtual void release() noexcept {}
    virtual Offer offer() const noexcept=0;
    virtual bool read(std::uint32_t offset,std::uint8_t*,std::size_t) noexcept=0;
    virtual ReadState poll_read(std::uint32_t offset,std::uint8_t* out,std::size_t size) noexcept {
        return read(offset,out,size)?ReadState::ready:ReadState::failed;
    }
    virtual void consumed(std::uint32_t) noexcept {}
    virtual bool healthy() const noexcept {return true;}
};
enum class State {idle,offering,waiting_ready,sending,stopping,complete,failed,cancelled};
// Driven by the existing worker/business owner, never a second serial loop.
class Session final : private device::McuCompletion {
public:
    explicit Session(device::Mcu& mcu) noexcept : mcu_(mcu) {}
    ~Session() {stop();}
    Session(const Session&)=delete;
    Session& operator=(const Session&)=delete;
    bool start(Image&,std::uint64_t now) noexcept;
    bool receive(const mcu::Frame&,std::uint64_t now) noexcept;
    void tick(std::uint64_t now) noexcept;
    void stop() noexcept;
    State state() const noexcept {return state_;}
    bool active() const noexcept;
private:
    void completed(device::McuRequest,bool,protocol::Bytes) noexcept override;
    void fail() noexcept;
    void cancel_request() noexcept;
    void release_image() noexcept;
    ReadState load_piece() noexcept;
    void deadline(unsigned delay) noexcept;
    device::Mcu& mcu_;
    Image* image_=nullptr;
    Offer offer_{};Ready ready_{};BinaryPlan plan_{};
    std::array<std::uint8_t,mcu::payload_limit> payload_{};
    std::size_t payload_size_=0,data_size_=0;
    std::uint32_t offset_=0;
    std::uint16_t index_=1,flags_=0;
    std::uint64_t now_=0,deadline_=0;
    bool offered_=false,pending_=false,loaded_=false,last_=false,negotiated_=false;
    State state_=State::idle;
};
}
