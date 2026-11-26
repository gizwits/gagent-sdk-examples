#pragma once
#include "gagent/cloud/cloud.hpp"
#include "gagent/ota/mcu_ota.hpp"
namespace gagent::cloud {
inline constexpr std::size_t download_block_limit=2048;
// restart carries no new identity: discard prior bytes, reset using the original
// Offer and acknowledge via resume_download() before receiving another byte.
enum class DownloadKind {metadata,data,complete,failed,restart};
struct DownloadEvent {
    std::uint32_t token=0;DownloadKind kind=DownloadKind::failed;int status=0;
    ota::Offer image{}; // Only valid for metadata, always delivered before any body data.
    std::array<std::uint8_t,512> bytes{};std::size_t size=0;
};
class Download {
public:
    virtual ~Download()=default;
    // Configure the next idle transfer for a slow downstream UART. Network reads
    // must still time out; consumer backpressure must not impose a total time cap.
    virtual void consumer_paced(bool) noexcept {}
    // GET only, exact nonzero image size. Owner must consume the terminal event before reuse.
    virtual bool begin_download(const Address&,const Request&,std::uint32_t size,std::uint32_t token) noexcept=0;
    // Legacy cloud download: size/MD5/type arrive in response headers, bounded by capacity.
    virtual bool begin_image_download(const Address&,const Request&,std::uint32_t capacity,std::uint32_t token) noexcept=0;
    // After metadata or restart, prepare storage then explicitly release body reception. Token must match.
    virtual bool resume_download(std::uint32_t token) noexcept=0;
    virtual bool poll_download(DownloadEvent&) noexcept=0;
    // Data view is owned by the source/event until the next poll or begin.
    // Consumers must finish using it synchronously before calling either.
    virtual bool poll_download_view(DownloadEvent& event,protocol::Bytes& bytes) noexcept {
        if(!poll_download(event))return false;
        bytes=event.size<=event.bytes.size()?protocol::Bytes{event.bytes.data(),event.size}:protocol::Bytes{};
        return true;
    }
    virtual std::size_t download_block_size() const noexcept {return 512;}
    virtual void cancel_download(std::uint32_t token) noexcept=0;
};
}
