#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace gagent::diagnostics {
struct Config {
    bool enabled=true;
    // Plain HTTP on its default port; configure a hostname, without a URL/port.
    const char* server_host="sdkdevops.gizwitsapi.com";
};
bool valid_server_host(std::string_view) noexcept;
// Every operation is bounded: network calls never wait, flash operations cover
// at most one sector. The runtime owner is also the sole OTA/provisioning owner.
class Port {
public:
    virtual ~Port()=default;
    virtual bool read(std::size_t,void*,std::size_t) noexcept=0;
    virtual std::uint32_t crc(std::uint32_t,const void*,std::size_t) noexcept=0;
    virtual int connect() noexcept=0; // 0 pending, 1 connected, -1 failed
    virtual int send(const void*,std::size_t) noexcept=0; // -2 would block
    virtual int receive(void*,std::size_t) noexcept=0; // -2 would block; 0 EOF
    virtual void close() noexcept=0;
    virtual bool erase_sector(std::size_t) noexcept=0;
    virtual void mark_empty() noexcept=0;
};
enum class Result {pending,empty,invalid,failed,uploaded};
class Upload {
public:
    Upload(std::uint64_t now,std::size_t capacity,std::uint16_t chip) noexcept
        :deadline_(now+15000),capacity_(capacity),chip_(chip) {}
    // Only printable trusted build/device metadata; no dynamic request allocation.
    bool request(const Config&,std::string_view mac,std::string_view version,std::string_view target) noexcept;
    Result step(Port&,std::uint64_t now) noexcept;
    bool acknowledged() const noexcept {return phase_==Phase::erase;}
    bool expired(std::uint64_t now) const noexcept {return now>=deadline_;}
private:
    enum class Phase {header,checksum,elf,program,note,connect,request,body,response,erase};
    Result response(Port&,std::uint64_t) noexcept;
    bool parse_headers() noexcept;
    bool parse_ack() const noexcept;
    std::uint64_t deadline_,erase_due_=0;
    std::size_t capacity_,size_=0,offset_=0,buffered_=0,sent_=0,text_size_=0,response_size_=0,body_left_=0;
    std::uint32_t checksum_=0,expected_=0;
    std::uint16_t chip_;
    std::uint16_t programs_left_=0,notes_left_=256;
    std::size_t program_at_=0,note_at_=0,note_end_=0;
    Phase phase_=Phase::header;
    bool headers_done_=false;
    std::array<std::uint8_t,512> chunk_{};
    std::array<char,1024> text_{};
    std::array<char,65> crash_sha_{};
};
// Main task owns scheduling. A connection flap never resets the retry budget.
class Schedule {
public:
    bool ready(std::uint64_t now,bool eligible) noexcept {
        if(!eligible){stable_=0;return false;}
        if(!stable_)stable_=now;
        return !finished_ && attempts_<3 && now>=due_ && now-stable_>=30000;
    }
    void started() noexcept {++attempts_;}
    void failed(std::uint64_t now) noexcept {due_=now+(attempts_<2?600000:3600000);}
    void finish() noexcept {finished_=true;}
private:
    std::uint64_t stable_=0,due_=0;
    unsigned attempts_=0;
    bool finished_=false;
};
}
