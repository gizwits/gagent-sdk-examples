#pragma once
#include "gagent/cloud/cloud.hpp"
#include <memory>
namespace gagent::cloud {
// Provider codecs contain wire details; the shared service owns the HTTP loan,
// timeout, cancellation and caller-owned file lifetime. All calls are Worker-only.
struct FileUploadRequest {
    Address address{};
    Request request{};
    std::array<char,2048> prefix{};
    std::array<char,96> suffix{};
    std::size_t prefix_size=0,suffix_size=0;
};
class FileUploadProtocol {
public:
    virtual ~FileUploadProtocol()=default;
    virtual bool prepare(const storage::Snapshot&,std::uint64_t unix_seconds,
        std::string_view filename,std::size_t size,std::string_view md5,
        std::string_view mime,FileUploadRequest&) noexcept=0;
    virtual bool response(int status,std::string_view response,char* url,std::size_t capacity) noexcept=0;
};
enum class FileUploadState { idle, uploading, draining, succeeded, failed, cancelled };
class FileUpload {
public:
    FileUpload(Service& cloud,Http& http) noexcept:cloud_(cloud),http_(http) {}
    FileUpload(const FileUpload&)=delete;
    FileUpload& operator=(const FileUpload&)=delete;
    // No copying/ownership transfer of file bytes. Keep bytes and codec alive
    // and immutable until busy()==false, ALSO after cancel() and bounded retries.
    // No Flash writes.
    // md5 is the digest of these exact bytes, computed off the Worker for images.
    bool begin(FileUploadProtocol&,const storage::Snapshot&,std::uint64_t unix_seconds,
        std::string_view filename,protocol::Bytes file,std::string_view md5,
        std::uint64_t now,std::uint32_t timeout_ms=120000,std::string_view mime="application/octet-stream") noexcept;
    void tick(std::uint64_t now) noexcept;
    void cancel() noexcept;
    bool busy() const noexcept {return state_==FileUploadState::uploading || state_==FileUploadState::draining;}
    FileUploadState state() const noexcept {return state_;}
    std::string_view url() const noexcept {return state_==FileUploadState::succeeded?std::string_view(url_.data()):std::string_view{};}
private:
    static constexpr unsigned max_attempts_=3;
    void retry_or_fail(std::uint64_t now) noexcept;
    void end(FileUploadState) noexcept;
    void drain() noexcept;
    Service& cloud_;Http& http_;
    FileUploadProtocol* codec_=nullptr;
    std::unique_ptr<FileUploadRequest> request_;
    protocol::Bytes file_{};
    std::array<char,640> url_{};
    std::uint64_t deadline_=0;
    std::uint64_t retry_at_=0;
    std::uint32_t loan_=0,token_=0;
    unsigned attempts_=0;
    FileUploadState state_=FileUploadState::idle,terminal_=FileUploadState::failed;
    bool pending_=false;
};
}
