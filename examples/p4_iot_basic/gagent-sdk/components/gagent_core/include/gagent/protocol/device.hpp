#pragma once
#include "gagent/protocol/mcu.hpp"
#include "gagent/application/board.hpp"
#include "gagent/storage/storage.hpp"
#include "gagent/protocol/mcu_handshake.hpp"
#include "gagent/protocol/mcu_baud.hpp"
#include <memory>
namespace gagent::runtime { class McuBootstrap; }
namespace gagent::device {
struct Request { protocol::Channel channel; std::uint32_t session,sequence; bool sequenced; protocol::PayloadFormat format=protocol::PayloadFormat::binary; };
class Completion { public: virtual ~Completion()=default; virtual void complete(const Request&,protocol::Bytes,bool success) noexcept=0; };
class Endpoint : public protocol::Endpoint {
public:
    void completion(Completion* target) noexcept { completion_=target; }
    virtual bool configure(const storage::Snapshot&) noexcept {return true;}
    virtual void advance(std::uint64_t) noexcept {}
    virtual bool activate(std::uint64_t) noexcept {return true;}
    virtual bool available() const noexcept {return true;}
    virtual void serial(protocol::Bytes) noexcept {}
    virtual void tick(std::uint64_t) noexcept {}
    virtual void disconnect(protocol::Channel,std::uint32_t) noexcept {}
    virtual void stop() noexcept {}
protected: Completion* completion_=nullptr;
};
struct Broadcast {std::array<std::uint32_t,4> sessions{};std::size_t count=0;bool failed=false;};
// Legacy small loop includes both LAN TCP and BLE; the large loop is MQTT.
enum class ReportScope : std::uint8_t {none=0,local=1,cloud=2,all=3,lan=4,bluetooth=8};
class Acknowledgements {
public:
    virtual ~Acknowledgements()=default;
    virtual protocol::Status acknowledge(const protocol::Context&,const protocol::Frame&) noexcept=0;
};
// Channel adapters own authorization and copying; one router owns control response semantics.
class Route : public protocol::Reply {
public:
    virtual Broadcast broadcast(const protocol::Frame&) noexcept {return {};}
    virtual void failed(std::uint32_t session) noexcept=0;
};
class Router final : public Completion,public protocol::Reply {
public:
    explicit Router(Endpoint& endpoint) noexcept : endpoint_(endpoint),dispatcher_(endpoint,*this) {endpoint_.completion(this);}
    Router(const Router&)=delete;
    Router& operator=(const Router&)=delete;
    bool bind(protocol::Channel,Route&) noexcept;
    protocol::Status dispatch(const protocol::Context&,const protocol::Frame&) noexcept;
    bool acknowledgements(Acknowledgements& target) noexcept {if(acknowledgements_ && acknowledgements_!=&target)return false;acknowledgements_=&target;return true;}
    std::array<Broadcast,3> broadcast(const protocol::Frame&,ReportScope scope=ReportScope::all) noexcept;
    protocol::Status send(const protocol::Context&,const protocol::Frame&) noexcept override;
    void complete(const Request&,protocol::Bytes,bool) noexcept override;
    void disconnect(protocol::Channel channel,std::uint32_t session) noexcept {endpoint_.disconnect(channel,session);}
private:
    Endpoint& endpoint_;protocol::Dispatcher dispatcher_;
    Acknowledgements* acknowledgements_=nullptr;
    std::array<Route*,3> routes_{};
    std::array<std::uint8_t,mcu::payload_limit+4> response_{};
    bool completing_=false;
};
using BoardControl=protocol::Status (*)(protocol::Bytes,std::uint8_t*,std::size_t,std::size_t&) noexcept;
class Soc final : public Endpoint {
public:
    explicit Soc(BoardControl handler) noexcept : handler_(handler) {}
    protocol::Status control(const protocol::Context&,const protocol::Control&) noexcept override;
private:
    BoardControl handler_;
    std::array<std::uint8_t,mcu::payload_limit> reply_{};
};
class Serial {
public:
    virtual ~Serial()=default;
    // Atomically copy into a bounded transmit buffer, or reject without retaining any data.
    virtual bool send(protocol::Bytes) noexcept=0;
    virtual void clear() noexcept=0;
    // True only after all accepted bytes have physically left the transmitter.
    virtual bool idle() const noexcept {return false;}
    // Zero advertises a fixed/unsupported adapter. Reconfigure only at physical TX idle.
    virtual std::uint32_t baudrate() const noexcept {return 0;}
    virtual bool reconfigure_baud(std::uint32_t) noexcept {return false;}
};
class ProductReceiver {
public:
    virtual ~ProductReceiver()=default;
    virtual bool product(protocol::Bytes,std::uint64_t) noexcept=0;
};
// Unsolicited MCU commands use one business owner. Views expire on return.
class McuHandler {
public:
    virtual ~McuHandler()=default;
    virtual void receive(const mcu::Frame&,std::uint64_t now) noexcept=0;
    virtual bool product(protocol::Bytes,std::uint64_t) noexcept {return false;}
    virtual void ready(std::uint64_t) noexcept {}
    virtual void link_failed(std::uint64_t) noexcept {}
    // True keeps the isolated bootstrap alive while this owner drains and restarts.
    // stop() cancels it; completion is observable if a host restart substitute returns.
    virtual bool startup_failed(std::uint64_t) noexcept {return false;}
    virtual bool restart_complete() const noexcept {return false;}
    // Bootstrap must drain accepted commands before installing product configuration.
    virtual bool commands_pending() const noexcept {return false;}
    virtual bool baud_change_allowed() const noexcept {return !commands_pending();}
    virtual void advance(std::uint64_t) noexcept {}
    virtual void tick(std::uint64_t) noexcept {}
    virtual void stopped() noexcept {}
};
// Outgoing management transactions share the existing serial sequence/retry owner.
// Add retained commands here with their payload and ACK contracts, never an independent UART loop.
enum class McuRequest : std::uint8_t { status=0x0d, ota_offer=0x19, ota_piece=0x1d, ota_stop=0x1f, time=0x57, voice_command=0x68
#ifdef GAGENT_CAMERA_ENABLED
    ,camera_result=0x77
#endif
};
class McuCompletion {
public:
    virtual ~McuCompletion()=default;
    // Called exactly once for an accepted request. Borrowed payload expires on return.
    virtual void completed(McuRequest,bool success,protocol::Bytes) noexcept=0;
};
enum class McuState { stopped, handshake, ready, failed, waiting_product };
class Mcu final : public Endpoint, private mcu::Consumer {
public:
    static constexpr unsigned handshake_attempt_limit=21;
    // Waiting controls own only their actual payload, with both count and byte bounds.
    static constexpr std::size_t control_queue_limit=16,control_queue_bytes_limit=4096;
    static constexpr std::uint32_t control_queue_wait_ms=5000;
    Mcu(Serial& serial,const board::Profile& profile) noexcept : serial_(serial),profile_(profile),active_baud_(profile.baud) {}
    void baud_storage(runtime::Boot& boot) noexcept {baud_boot_=&boot;}
    std::uint32_t baudrate() const noexcept {return active_baud_;}
    bool baud_busy() const noexcept {return baud_.phase!=BaudPhase::idle || baud_.reconnect;}
    const mcu::Dialect& dialect() const noexcept {return information_.format==mcu::HandshakeFormat::key_value?mcu::key_value_dialect():mcu::fixed_dialect;}
    bool configure(const storage::Snapshot&) noexcept override;
    // Before activation, or after a completed bootstrap handshake while still isolated.
    bool timing(std::uint32_t heartbeat_ms,std::uint32_t ack_timeout_ms) noexcept;

    void advance(std::uint64_t now) noexcept override {if(now>=now_){now_=now;if(handler_ && (!probing_ || commands_available()))handler_->advance(now);}}
    bool start(std::uint64_t now) noexcept;
    bool probe(std::uint64_t now,const storage::Snapshot* saved=nullptr) noexcept;
    bool resume(std::uint64_t now) noexcept;
    // Application worker suspends business before installing a validated live handshake.
    bool suspend_product() noexcept;
    bool install_product(protocol::Bytes) noexcept;
    bool probing() const noexcept {return probing_;}
    bool activate(std::uint64_t now) noexcept override {return start(now);}
    bool available() const noexcept override {return state_==McuState::ready && !probing_ && !baud_busy();}
    // Receiving MCU requests is independent of exposing a validated network endpoint.
    bool commands_available() const noexcept {return !baud_busy() && (available() || (waiting_commands_ && probing_ && (state_==McuState::waiting_product || state_==McuState::ready)));}
    void serial(protocol::Bytes b) noexcept override {feed(b);}
    protocol::Status feed(protocol::Bytes bytes) noexcept { return decoder_.feed(bytes,*this); }
    protocol::Status control(const protocol::Context&,const protocol::Control&) noexcept override;
    void tick(std::uint64_t now) noexcept override;
    void disconnect(protocol::Channel,std::uint32_t session) noexcept override;
    void stop() noexcept override;
    // Copies the request, or rejects without retaining the completion owner. One transaction at a time.
    // The completion owner must outlive the accepted request, including stop/quiesce cancellation.
    bool request(McuRequest,protocol::Bytes,McuCompletion&,std::uint16_t flags=0) noexcept;
    // Cancel only this owner; never clears unrelated requests or already accepted UART bytes.
    bool cancel(McuCompletion&) noexcept;
    void handler(McuHandler& handler) noexcept {handler_=&handler;}
    // Copies a response independently of the outgoing request/retry frame. No silent eviction.
    bool reply(const mcu::Frame&) noexcept;
    std::size_t queued_replies() const noexcept {return reply_count_;}
    std::size_t queued_controls() const noexcept {return control_count_;}
    bool transaction_pending() const noexcept {return pending_ || control_count_;}
    void isolate() noexcept {quiesce();state_=McuState::failed;fault_notified_=false;if(handler_)handler_->stopped();}
    bool output_idle() const noexcept {return !control_count_ && !reply_count_ && serial_.idle();}
    void quiesce() noexcept;
    bool ota_enabled() const noexcept {return profile_.mode==board::Mode::mcu && profile_.mcu_ota;}
    std::uint64_t now() const noexcept {return now_;}
    McuState state() const noexcept { return state_; }
    protocol::Bytes handshake() const noexcept {return information_.wire();}
    const mcu::HandshakeInfo& information() const noexcept {return information_;}
    mcu::HandshakeError handshake_error() const noexcept {return handshake_error_;}
private:
    friend class runtime::McuBootstrap;
    bool restore_probe(protocol::Bytes,std::uint64_t now) noexcept;
    bool wait_product() noexcept;
    bool fail_probe(std::uint64_t now) noexcept;
    bool probe_restart_complete() const noexcept {return handler_ && handler_->restart_complete();}
    void flush_reply() noexcept;
    bool enqueue_reply(const mcu::Frame&) noexcept;
    bool baud_start() noexcept;
    bool baud_receive(const mcu::Frame&) noexcept;
    void baud_tick() noexcept;
    void baud_product() noexcept;
    bool baud_reply(mcu::baud::Message,std::uint8_t sequence) noexcept;
    void baud_cancel() noexcept;
    mcu::baud::Result baud_save() noexcept;
    enum class BaudPhase : std::uint8_t {idle,prepare,drain,guard,verify,rollback};
    struct BaudSession {
        BaudPhase phase=BaudPhase::idle;
        std::uint32_t session=0,probe_token=0,token=0,wanted=0,previous=0;
        std::uint64_t deadline=0,retry_at=0;
        std::uint8_t sequence=0;
        bool reconnect=false,handshake_sent=false,announce=false,verified=false,completed=false;
        mcu::baud::Result result=mcu::baud::Result::ok;
    } baud_;
    runtime::Boot* baud_boot_=nullptr;
    std::uint32_t baud_counter_=0;
    std::uint32_t baud_failed_session_=0,baud_failed_rate_=0;
    bool forward_control(const Request&,protocol::Bytes) noexcept;
    void flush_control() noexcept;
    void expire_controls() noexcept;
    Request remove_control(std::size_t index) noexcept;
    void fail_controls() noexcept;
    struct QueuedControl {
        Request request{};
        std::unique_ptr<std::uint8_t[]> payload;
        std::size_t size=0;
        std::uint64_t accepted_at=0;
    };
    std::array<QueuedControl,control_queue_limit> controls_{};
    std::size_t control_count_=0,control_bytes_=0;
    struct Reply {std::array<std::uint8_t,544> bytes{};std::size_t size=0;};
    std::array<Reply,4> replies_{};
    std::size_t reply_head_=0,reply_count_=0;
    McuHandler* handler_=nullptr;
    McuCompletion* transaction_=nullptr;
    bool begin(std::uint8_t command,protocol::Bytes,std::uint16_t flags=0) noexcept;
    void received(const mcu::Frame&) noexcept override;
    void normalized(const mcu::Frame&,mcu::HandshakeFormat=mcu::HandshakeFormat::unknown) noexcept;
    void link_failure() noexcept;
    unsigned heartbeat_failures_=0;
    unsigned handshake_attempts_=0;
    bool fault_notified_=false;
    void checksum_failed(std::uint8_t command,std::uint8_t sequence) noexcept override;
    void error_reply(std::uint8_t sequence,std::uint8_t error) noexcept;
    void finish(bool success,protocol::Bytes = {},bool acknowledged=false) noexcept;
    Serial& serial_;
    board::Profile profile_;
    std::uint32_t active_baud_;
    mcu::Decoder decoder_;
    std::array<std::uint8_t,mcu::wire_limit> tx_{};
    mcu::HandshakeInfo information_{};
    mcu::HandshakeError handshake_error_=mcu::HandshakeError::none;
    bool format_locked_=false,handshake_seen_=false;
    std::array<std::uint64_t,256> reusable_{};
    std::size_t tx_size_=0;
    std::array<char,32> product_key_{};
    bool key_set_=false,secret_set_=false,encrypted_discovery_=false;
    Request request_{};
    std::uint64_t now_=0,deadline_=0,last_heartbeat_=0;
    std::uint32_t wait_ms_=0;
    std::uint8_t sequence_=0,command_=0,next_sequence_=1;
    unsigned retries_=0;
    bool pending_=false,has_request_=false,quiescing_=false,probing_=false,waiting_commands_=false;
    McuState state_=McuState::stopped;
};
}
