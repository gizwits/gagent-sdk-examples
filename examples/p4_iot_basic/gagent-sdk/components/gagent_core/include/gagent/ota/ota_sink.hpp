#pragma once
#include "gagent/ota/mcu_ota.hpp"
namespace gagent::ota {
enum class SinkState {empty,preparing,receiving,verifying,ready,failed};
// Download destination owned by the same worker as the HTTP controller.
// Preparation and verification advance at most one storage step per call.
class ImageSink {
public:
    virtual ~ImageSink()=default;
    virtual std::size_t capacity() const noexcept=0;
    virtual bool busy() const noexcept=0;
    virtual bool begin(const Offer&) noexcept=0;
    virtual bool prepare_step() noexcept=0;
    virtual bool append(protocol::Bytes) noexcept=0;
    // Bounded streaming destinations can pause the HTTP producer before polling.
    virtual bool consumer_paced() const noexcept {return false;}
    virtual bool accepts(std::size_t) const noexcept {return true;}
    virtual bool seal() noexcept=0;
    virtual bool verify_step() noexcept=0;
    virtual bool cancel() noexcept=0;
    virtual SinkState sink_state() const noexcept=0;
};
class ModuleImage : public ImageSink {
public:
    virtual bool activate() noexcept=0;
};

}
