#pragma once
#include "gagent/cloud/cloud.hpp"
#include "gagent/ota/download.hpp"
#include <atomic>
#include <cstdlib>
#include <memory>
#include <type_traits>
namespace gagent::platform {
// One bounded I/O job. Cloud HTTP and board-owned peripheral OTA share an
// exclusive temporary task slot; no concurrent HTTP client task is admitted.
// This object must outlive busy()==false, including after cancel (DNS/socket calls may still be returning).
class EspHttp final : public cloud::Http,public cloud::Download {
public:
    // malloc-compatible hooks allow deterministic workspace allocation failures.
    using Allocate=void*(*)(std::size_t);
    using Release=void(*)(void*);
    explicit EspHttp(Allocate allocate=std::malloc,Release release=std::free) noexcept
        : allocate_(allocate),workspace_(nullptr,release) {}
    // Run a peripheral OTA on the same exclusive I/O task slot as cloud HTTP.
    // Called only by the runtime owner; callback/context must outlive the job.
    using OtaJob = void (*)(void*, bool (*)(void*), void*) noexcept;
    bool begin_ota_job(OtaJob,void*) noexcept;
    bool ota_job_busy() const noexcept {return busy() && ota_job_active_.load();}
    bool begin_image_download(const cloud::Address&,const cloud::Request&,std::uint32_t capacity,std::uint32_t token) noexcept override;
    bool begin_download(const cloud::Address&,const cloud::Request&,std::uint32_t size,std::uint32_t token) noexcept override;
    void consumer_paced(bool enabled) noexcept override {if(idle())consumer_paced_=enabled;}
    bool resume_download(std::uint32_t token) noexcept override;
    bool poll_download(cloud::DownloadEvent&) noexcept override;
    bool poll_download_view(cloud::DownloadEvent&,protocol::Bytes&) noexcept override;
    std::size_t download_block_size() const noexcept override {return cloud::download_block_limit;}
    void cancel_download(std::uint32_t token) noexcept override;
    bool begin_borrowed(const cloud::Address&,const cloud::Request&,std::uint32_t token) noexcept override;
    bool begin_upload(const cloud::Address& address,const cloud::Request& request,
        const cloud::UploadBody& body,std::uint32_t token) noexcept override {
        return begin_upload_with_status(address,request,body,token)==cloud::HttpStart::started;
    }
    cloud::HttpStart begin_upload_with_status(const cloud::Address&,const cloud::Request&,const cloud::UploadBody&,std::uint32_t token) noexcept override;
    bool poll_borrowed(std::uint32_t token,cloud::HttpView&) noexcept override;
    void cancel_borrowed(std::uint32_t token) noexcept override;
    bool begin(const cloud::Address&,const cloud::Request&,std::uint32_t token) noexcept override;
    using cloud::Http::poll;
    bool poll(cloud::HttpView&) noexcept override;
    void cancel() noexcept override {cancelled_.store(true);}
    bool idle() const noexcept override {return !busy() && !done_.load() && !chunk_ready_.load();}
    // Reap the finished I/O task before exposing completion;
    // TLS cleanup alone does not release the task's 8 KiB stack.
    bool busy() const noexcept;
    // Worker-only idle hook. Worker notification index 0 is shared with BLE as
    // a wake hint; the temporary HTTP task uses its own index 0. The Worker must
    // drain this adapter before exiting, so its captured task handle stays live.
    void wait_worker(unsigned ticks) noexcept;
private:
    bool poll_download_into(cloud::DownloadEvent&,std::uint8_t*,std::size_t) noexcept;
    std::size_t chunk_offset_=0; // Worker-only cursor for legacy 512-byte consumers.
    void wake_producer() noexcept;
    void wake_worker() noexcept;
    bool start_task() noexcept;
    cloud::HttpStart launch(const cloud::Address&,const cloud::Request&,std::uint32_t token,std::uint32_t download_size,std::uint32_t image_capacity=0,bool borrowed=false,const cloud::UploadBody* upload=nullptr) noexcept;
    static void entry(void*) noexcept;
    void perform() noexcept;
    cloud::Address address_{};
    struct RequestWorkspace {
        cloud::Request request{};
        cloud::UploadBody upload{};
        std::int64_t upload_deadline_us=0;
        // Scheme, host, :port, path and terminator at their maximum capacities.
        std::array<char,8+254+6+512> url{};
    };
    static_assert(std::is_trivially_destructible_v<RequestWorkspace>);
    Allocate allocate_;
    // Worker creates before launch; producer frees after HTTP cleanup and before
    // publishing completion. Response storage separately owns returned views.
    std::unique_ptr<RequestWorkspace,Release> workspace_;
    cloud::HttpResponse response_{};
    bool upload_retryable_=false; // Published with the borrowed terminal response.
    // Streaming reuses response_.body/size; ordinary poll is isolated until the stream completes.
    std::uint32_t download_size_=0,image_capacity_=0;
    cloud::DownloadKind pending_kind_=cloud::DownloadKind::data;
    bool borrowed_=false; // Accessed only by the owner task, never the I/O task.
    bool consumer_paced_=false; // Set while idle, immutable while the I/O task runs.
    bool metadata_consumed_=false; // Owner task only.
    std::atomic<bool> body_ready_{false};
    std::atomic<bool> streaming_{false},chunk_ready_{false};
    mutable std::atomic<bool> busy_{false};
    std::atomic<bool> done_{false},cancelled_{false},finished_{false};
    mutable std::atomic<void*> task_{nullptr};
    void* worker_task_=nullptr;
    mutable std::atomic_flag reaping_=ATOMIC_FLAG_INIT;
    OtaJob ota_job_=nullptr;
    void* ota_job_context_=nullptr;
    mutable std::atomic<bool> ota_job_active_{false};
};
}
