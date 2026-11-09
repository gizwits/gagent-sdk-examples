#pragma once
#include "gagent/provisioning/airlink.hpp"
#include "esp_event.h"
namespace gagent::platform {
class EspAirlinkUdp final : public airlink::Transport {
public:
    EspAirlinkUdp()=default;
    ~EspAirlinkUdp() {close();}
    EspAirlinkUdp(const EspAirlinkUdp&)=delete;
    EspAirlinkUdp& operator=(const EspAirlinkUdp&)=delete;
    bool open(const std::array<std::uint8_t,4>&,std::uint16_t) noexcept override;
    int send(airlink::Peer,protocol::Bytes) noexcept override;
    int probe(airlink::Peer&) noexcept override;
    void close() noexcept override;
private:
    int fd_=-1;
};
using AirlinkCapture=airlink::Captured;
using CaptureState=airlink::CaptureState;
// Decoder only: the one radio owner must prepare an unassociated STA with cleared IP,
// suspend its station manager, and keep Wi-Fi/netif alive until stop/drain completes.
// One Worker calls methods. Callback storage is static, never a borrowed object address.
class EspAirlinkCapture final : public airlink::Decoder {
public:
    EspAirlinkCapture()=default;
    ~EspAirlinkCapture();
    EspAirlinkCapture(const EspAirlinkCapture&)=delete;
    EspAirlinkCapture& operator=(const EspAirlinkCapture&)=delete;
    bool start(std::uint64_t window,std::uint64_t now) noexcept override;
    CaptureState poll(std::uint64_t now) noexcept override;
    bool take(AirlinkCapture&) noexcept override;
    void stop(std::uint64_t now) noexcept override;
    CaptureState state() const noexcept override {return state_;}
private:
    bool detach() noexcept;
    void quarantine() noexcept;
    esp_event_handler_instance_t capture_handler_=nullptr,barrier_handler_=nullptr;
    CaptureState state_=CaptureState::idle;
    std::uint64_t now_=0,deadline_=0;
    unsigned epoch_=0;
    bool leased_=false,decoder_=false,posted_=false,error_=false,taken_=false;
};
}
