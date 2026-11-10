#pragma once
#include "gagent/diagnostics/runtime_logs.hpp"
#include "http_parser.h"
#include <array>
namespace gagent::diagnostics {
// A single frozen batch. All network operations return immediately; no task,
// request-body copy, HTTP client heap, or unbounded response accumulation.
class LogSocket {
public:
    virtual ~LogSocket()=default;
    virtual int connect(const char*,unsigned) noexcept=0; // -1 error, 0 pending, 1 ready
    virtual int send(const void*,std::size_t) noexcept=0; // -2 would block
    virtual int receive(void*,std::size_t) noexcept=0; // -2 would block, 0 EOF
};
class LogUpload {
public:
    enum class Result {pending,success,retry,permanent};
    bool begin(std::string_view url,std::string_view token,const LogIdentity&,
               std::string_view session,const LogCapture::Batch&,std::uint64_t now) noexcept;
    Result step(LogSocket&,std::uint64_t now) noexcept;
    std::uint64_t retry_ms() const noexcept {return retry_ms_;}
    unsigned http_status() const noexcept {return parser_.status_code;}
private:
    enum class Phase {connect,headers,body,response};
    Phase phase_=Phase::connect;
    std::array<char,1024> io_{}; // Request headers, then a <=256-byte receive chunk.
    std::array<char,384> body_{};
    std::array<char,254> host_{};
    std::array<char,24> field_{};
    std::array<char,32> value_{};
    std::size_t request_size_=0,sent_=0,body_size_=0,field_size_=0,value_size_=0,header_bytes_=0;
    unsigned port_=80;
    bool value_started_=false,header_overflow_=false,headers_done_=false,complete_=false,retry_header_=false;
    std::uint64_t deadline_=0,retry_ms_=5000;
    LogCapture::Batch batch_{};
    http_parser parser_{};
    void finish_header() noexcept;
    Result acknowledge() const noexcept;
    static const http_parser_settings settings_;
};
}
