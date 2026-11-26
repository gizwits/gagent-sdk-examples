#pragma once
#include "gagent/protocol/protocol.hpp"
#include <array>
#include <algorithm>
namespace gagent::ota {
// Data-point extension enums. In particular voice=3 is NOT Target::voice (7).
enum class StatusType : std::uint8_t {module=1,mcu=2,voice=3};
enum class Phase : std::uint8_t {idle,preparing,downloading,verifying,verified,installing,rebooting,confirming,succeeded,failed,unavailable,transferring,streaming};
enum class Reason : std::uint16_t {
    none=0,unknown=1,invalid_info=2,product_mismatch=3,hardware_mismatch=4,version_rejected=5,
    storage=6,power=7,download=8,download_timeout=9,size=10,checksum=11,signature=12,image=13,
    erase=14,write=15,communication=16,activation_timeout=17,boot_confirmation=18,rollback=19,cancelled=20,interrupted=21,
    target_not_found=0x101,type_unsupported=0x102,offline=0x103,query_timeout=0x104,state_unavailable=0x105
};
struct StatusSnapshot {
    std::uint32_t sdid=0;
    StatusType type=StatusType::module;
    Phase phase=Phase::idle;
    std::uint8_t progress=0xff;
    Reason reason=Reason::none;
    std::array<std::uint8_t,12> encode() const noexcept {
        return {0x15,std::uint8_t(sdid>>24),std::uint8_t(sdid>>16),std::uint8_t(sdid>>8),std::uint8_t(sdid),1,
            std::uint8_t(type),std::uint8_t(phase),progress,std::uint8_t(phase==Phase::succeeded?1:phase==Phase::failed?2:0),
            std::uint8_t(unsigned(reason)>>8),std::uint8_t(reason)};
    }
};
inline protocol::Status status_query(protocol::Bytes bytes,StatusSnapshot& out) noexcept {
    if(!bytes.data || bytes.size!=7 || bytes.data[0]!=0x16)return protocol::Status::malformed;
    if(bytes.data[5]!=1 || bytes.data[6]<1 || bytes.data[6]>3 || (bytes.data[1]&0x80))return protocol::Status::unsupported;
    out={};for(unsigned i=1;i<=4;++i)out.sdid=(out.sdid<<8)|bytes.data[i];
    out.type=StatusType(bytes.data[6]);return protocol::Status::ok;
}
// 95% is byte-work completion; only independently confirmed success is 100%.
// Count committed ingress and target ACKs, never retries or UART bytes sent.
inline std::uint8_t overall_progress(std::uint32_t size,std::uint32_t received,std::uint32_t confirmed,bool peripheral) noexcept {
    if(!size)return 0;
    const auto work=std::uint64_t(std::min(received,size))+(peripheral?std::min(confirmed,size):0);
    return std::uint8_t(95*work/(std::uint64_t(size)*(peripheral?2:1)));
}
class StatusTracker {
public:
    void begin(StatusType type,std::uint8_t initial=0,Phase phase=Phase::preparing) noexcept {current_={0,type,phase,std::uint8_t(std::min<unsigned>(initial,95)),Reason::none};active_=true;sent_=false;}
    void update(Phase phase,std::uint8_t progress,Reason reason=Reason::none) noexcept {
        if(!active_)return;
        current_.phase=phase;current_.reason=reason;
        // Re-downloads/recovery keep the last proven high-water mark, never advance by time.
        if(progress<=100)current_.progress=std::max(current_.progress, std::uint8_t(std::min<unsigned>(progress,phase==Phase::succeeded?100:95)));
        if(phase==Phase::succeeded)current_.progress=100;
    }
    bool active() const noexcept {return active_;}
    const StatusSnapshot& current() const noexcept {return current_;}
    StatusSnapshot query(StatusType type) const noexcept {
        if(active_ && !terminal())return current_;
        return {0,type,Phase::idle,0xff,Reason::none};
    }
    bool due(std::uint64_t now) const noexcept {
        if(!active_)return false;
        if(!sent_ || current_.phase!=last_.phase || current_.reason!=last_.reason)return true;
        if(now<sent_at_)return false;
        const auto elapsed=now-sent_at_;
        return elapsed>=20000 || (elapsed>=5000 && unsigned(current_.progress)>=unsigned(last_.progress)+5);
    }
    void sent(std::uint64_t now) noexcept {last_=current_;sent_at_=now;sent_=true;if(terminal())active_=false;}
private:
    bool terminal() const noexcept {return current_.phase==Phase::succeeded || current_.phase==Phase::failed;}
    StatusSnapshot current_{},last_{};
    std::uint64_t sent_at_=0;
    bool active_=false,sent_=false;
};
}
