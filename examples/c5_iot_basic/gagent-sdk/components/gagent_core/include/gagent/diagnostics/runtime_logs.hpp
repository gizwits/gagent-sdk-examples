#pragma once
#include "gagent/protocol/protocol.hpp"
#include <array>
#include <string_view>

namespace gagent::diagnostics {
struct LogIdentity {
    std::string_view product_key, device_id, firmware;
    std::array<std::uint8_t,6> mac{};
};
class RuntimeLogs {
public:
    virtual ~RuntimeLogs()=default;
    // Only called by the application owner after MQTT topic/epoch validation.
    // Implementations copy identity before returning; all storage is lazy.
    virtual bool enable(const LogIdentity&) noexcept=0;
    virtual void stop() noexcept=0;
};
// Validated legacy 0x0218 payload, independent of the cloud-specific envelope.
protocol::Status log_command(const protocol::Frame&,bool& enabled) noexcept;
std::size_t normalize_log(char* output,std::size_t capacity,const char* input,std::size_t size) noexcept;

class LogCapture {
public:
#ifdef GAGENT_C2_MEMORY_PROFILE
    static constexpr std::size_t capacity=1024;
#else
    static constexpr std::size_t capacity=4096;
#endif
    static constexpr std::uint64_t duration_ms=3600000;
    enum class Action { wait, send, done };
    struct Batch {const char* data;std::size_t size;std::uint32_t sequence,dropped;bool final;};
    explicit LogCapture(std::uint64_t now) noexcept: deadline_(now+duration_ms),last_sent_(now) {}
    bool renew(std::uint64_t now) noexcept {if(!collecting_ || finished_)return false;deadline_=now+duration_ms;return true;}
    void stop(std::uint64_t now) noexcept {if(collecting_){collecting_=false;stopped_=now;}}
    bool collecting() const noexcept {return collecting_ && !finished_;}
    void append(const char*,std::size_t) noexcept;
    void dropped() noexcept {if(collecting() && dropped_<1000000000)++dropped_;}
    Action poll(std::uint64_t now) noexcept;
    Batch batch() const noexcept {return {pending_.data(),pending_size_,sequence_,pending_dropped_,final_};}
    void complete(std::uint64_t now,bool success,bool permanent=false,std::uint64_t retry_ms=5000) noexcept;
    bool failed() const noexcept {return failed_;}
private:
    std::array<char,capacity> current_{},pending_{};
    std::size_t used_=0,pending_size_=0;
    std::uint64_t deadline_=0,last_sent_=0,stopped_=0,retry_at_=0;
    std::uint32_t sequence_=0,dropped_=0,pending_dropped_=0;
    unsigned attempts_=0;
    bool collecting_=true,pending_ready_=false,final_=false,finished_=false,failed_=false;
};
}
