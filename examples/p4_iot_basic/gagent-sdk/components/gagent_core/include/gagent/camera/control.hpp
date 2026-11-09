#pragma once
#include "gagent/protocol/mcu.hpp"
#include "gagent/storage/storage.hpp"
#include <array>
namespace gagent::camera {
// Compiled only for a camera-enabled board. No media types cross this boundary.
enum class State : std::uint8_t { idle, detecting, starting, streaming, paused, failed };
struct Status {
    State state=State::idle;
    std::uint8_t model=0,score=0;
    bool present=false,online=false,detection_enabled=false;
    std::uint32_t error=0,event=0;
    std::uint16_t x=0,y=0,width=0,height=0;
};
class Control {
public:
    virtual ~Control()=default;
    // Owner-worker calls only; implementations must copy borrowed identity before returning.
    virtual void sync(bool allowed,bool online,const storage::Snapshot*,const std::array<std::uint8_t,6>& mac) noexcept=0;
    // 1=start stream, 2=stop, 3=enable detection, 4=disable detection.
    // 0 means queued, never proof that a stream connected.
    virtual std::uint8_t request(std::uint8_t operation) noexcept=0;
    virtual Status status() const noexcept=0;
    virtual bool idle() const noexcept=0;
};
// One command owner; bounded idempotency cache prevents retransmissions from
// re-enqueuing accepted operations. Query always returns a fresh status snapshot.
class Protocol {
public:
    static constexpr std::uint8_t request_command=0x75,response_command=0x76,
                                  notification_command=0x77,ack_command=0x78,notification_operation=0x80;
    using Reply=std::array<std::uint8_t,24>;
    void attach(Control* control) noexcept {control_=control;}
    Reply receive(const mcu::Frame& frame,std::uint64_t now,bool blocked) noexcept {
        Reply reply{};reply[0]=1;
        const bool valid=!frame.flags && frame.payload.data && frame.payload.size==2 && frame.payload.data[0]==1 && frame.payload.data[1]<=4;
        const auto op=frame.payload.data && frame.payload.size>=2?frame.payload.data[1]:std::uint8_t(0xff);
        reply[1]=op;reply[2]=valid?0:1;
        if(valid){
            if(!control_)reply[2]=4;
            else if(op){
                bool cached=false;
                for(const auto& entry:history_)if(entry.valid && now<entry.expires && entry.sequence==frame.sequence){
                    reply[2]=entry.operation==op?0:1;cached=true;break;
                }
                if(!cached){
                    reply[2]=blocked?3:control_->request(op);
                    if(!reply[2]){history_[next_]={now+5000,frame.sequence,op,true};next_=(next_+1)%history_.size();}
                }
            }
        }
        encode_status(reply,control_?control_->status():Status{});
        return reply;
    }
    // Snapshots coalesce while the shared UART is busy. Never queue video frames
    // or an unbounded series of inference results behind MCU/OTA traffic.
    bool notification(std::uint64_t now,Reply& reply) const noexcept {
        if(!control_ || now<retry_after_)return false;
        reply={};reply[0]=1;reply[1]=notification_operation;
        encode_status(reply,control_->status());
        if(!notice_sent_)return true;
        if(now<notice_at_ || now-notice_at_<1000)return false;
        return reply!=last_notice_ || now-notice_at_>=10000;
    }
    void notification_sent(std::uint64_t now,const Reply& reply) noexcept {
        last_notice_=reply;notice_at_=now;notice_sent_=true;
    }
    void notification_failed(std::uint64_t now) noexcept {retry_after_=now+5000;}
    void reset() noexcept {history_={};next_=0;notice_sent_=false;notice_at_=retry_after_=0;last_notice_={};}
private:
    static void encode_status(Reply& reply,const Status& value) noexcept {
        reply[3]=static_cast<std::uint8_t>(value.state);reply[4]=value.model;
        reply[5]=value.present;reply[6]=value.score;
        reply[7]=(value.online?1:0)|(value.detection_enabled?2:0);
        auto be=[&](std::size_t at,std::uint32_t v,unsigned size){for(unsigned i=0;i<size;++i)reply[at+i]=std::uint8_t(v>>(8*(size-1-i)));};
        be(8,value.error,4);be(12,value.event,4);be(16,value.x,2);be(18,value.y,2);be(20,value.width,2);be(22,value.height,2);
    }
    Reply last_notice_{};
    std::uint64_t notice_at_=0,retry_after_=0;
    bool notice_sent_=false;
    struct Entry {std::uint64_t expires=0;std::uint8_t sequence=0,operation=0;bool valid=false;};
    std::array<Entry,4> history_{};
    std::size_t next_=0;
    Control* control_=nullptr;
};
}
