#pragma once
#include "gagent/ota/download.hpp"
#include "gagent/ota/ota_sink.hpp"
namespace gagent::ota {
enum class DownloadState {idle,headers,preparing,receiving,verifying,ready,cancelling,failed,restarting};
// One business worker owns this controller and its image destination. The caller owns the HTTP loan
// until draining()==false, including after cancellation. No SDK callbacks touch storage.
class DownloadController {
public:
    DownloadController(cloud::Download& source,ImageSink& sink) noexcept:source_(&source),sink_(sink) {}
    DownloadController(cloud::Download* source,ImageSink& sink) noexcept:source_(source),sink_(sink) {}
    bool begin_image(const Offer&) noexcept;
    bool append(protocol::Bytes) noexcept;
    bool seal() noexcept;
    bool begin(const cloud::Address&,const cloud::Request&,std::uint32_t token) noexcept;
    bool begin(const cloud::Address&,const cloud::Request&,std::uint32_t token,const Offer&) noexcept;
    void tick() noexcept;
    // For paced sinks, the owner must stop the old MCU session before restart.
    bool restart() noexcept;
    void cancel() noexcept;
    DownloadState state() const noexcept {return state_;}
    Reason failure_reason() const noexcept {return reason_;}
    std::uint32_t received() const noexcept {return received_;}
    std::uint32_t image_size() const noexcept {return image_size_;}
    bool draining() const noexcept {return pending_ || provided_offer_;}
private:
    void fail(Reason reason=Reason::unknown) noexcept;
    Reason reason_=Reason::none;
    void note_progress(std::size_t) noexcept;
    bool active() const noexcept;
    cloud::Download* source_;ImageSink& sink_;
    cloud::DownloadEvent event_{};Offer offer_{};
    std::uint32_t token_=0,received_=0,image_size_=0;
    unsigned logged_tenth_=0;
    DownloadState state_=DownloadState::idle;
    bool pending_=false,provided_offer_=false;
    cloud::Address address_{};cloud::Request request_{};
};
}
