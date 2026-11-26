#pragma once
#include "gagent/ota/ota_verify.hpp"
#include "gagent/ota/ota_sink.hpp"
#include "esp_ota_ops.h"
#include "gagent/ota/partition_lease.hpp"
#include "gagent/ota_timing.hpp"
namespace gagent::platform {
enum class ModuleOtaState {idle,writing,verifying,ready,selected,failed};
// Worker-owned SDK OTA handle. Validation and boot selection are separate operations.
class ModuleOta final : public ota::ModuleImage {
public:
    ModuleOta(ota::Digest& digest,bool enabled) noexcept:verifier_(digest),enabled_(enabled) {}
    ~ModuleOta(){cancel();}
    ModuleOta(const ModuleOta&)=delete;
    ModuleOta& operator=(const ModuleOta&)=delete;
    std::size_t capacity() const noexcept override;
    bool begin(const ota::Offer&) noexcept override;
    bool append(protocol::Bytes) noexcept override;
    bool finish() noexcept;
    bool verify_step() noexcept override;
    bool activate() noexcept override;
    bool cancel() noexcept override;
    bool busy() const noexcept override {
        return live_ || selection_attempted_ || state_==ModuleOtaState::verifying || state_==ModuleOtaState::ready;
    }
    bool prepare_step() noexcept override {return state_==ModuleOtaState::writing;}
    bool seal() noexcept override {return finish();}
    ota::SinkState sink_state() const noexcept override {
        switch(state_){
        case ModuleOtaState::idle:return ota::SinkState::empty;
        case ModuleOtaState::writing:return ota::SinkState::receiving;
        case ModuleOtaState::verifying:return ota::SinkState::verifying;
        case ModuleOtaState::ready:case ModuleOtaState::selected:return ota::SinkState::ready;
        case ModuleOtaState::failed:return ota::SinkState::failed;
        }
        return ota::SinkState::failed;
    }
    ota::Reason failure_reason() const noexcept override {return reason_;}
    ModuleOtaState state() const noexcept {return state_;}
private:
    const esp_partition_t* candidate() const noexcept;
    bool fail(ota::Reason reason=ota::Reason::unknown) noexcept;
    ota::Reason reason_=ota::Reason::unknown;
    ota::PartitionLease partition_lease_;
    ota::Verifier verifier_;ota::Offer offer_{};
#if GAGENT_OTA_TIMING
    OtaTimingWindow timing_;
    std::int64_t flash_us_=0,verify_us_=0;
#endif
    const esp_partition_t* partition_=nullptr;
    esp_ota_handle_t handle_=0;std::size_t written_=0;
    bool enabled_=false,live_=false,selection_attempted_=false;
    ModuleOtaState state_=ModuleOtaState::idle;
};
}
