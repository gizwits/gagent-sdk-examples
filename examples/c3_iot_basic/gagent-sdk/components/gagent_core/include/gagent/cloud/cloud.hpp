#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/protocol/protocol.hpp"
#include "gagent/cloud/credential_recovery.hpp"
namespace gagent::mqtt {class Driver;}
namespace gagent::ota {struct CheckMetadata;struct CheckReply;}
namespace gagent::cloud {
// Built-in providers track their actual persisted inputs. Custom providers retain
// the conservative whole-snapshot behavior unless they explicitly select a scope.
struct ConfigurationScope {
    const std::string_view* keys=nullptr;
    std::size_t count=0;
    std::string_view (*value)(const storage::Snapshot&,std::string_view) noexcept=nullptr;
    bool (*valid)(const storage::Snapshot&) noexcept=nullptr;
    // Opt in only with immutable keys and pure snapshot-based callbacks.
    // Dynamic/custom providers keep recomputing by default.
    bool cacheable=false;
};
bool fingerprint(const storage::Snapshot&,storage::Md5,std::array<std::uint8_t,16>&,ConfigurationScope={}) noexcept;
enum class Operation { registration, provision, deregistration };
constexpr const char* operation_name(Operation op) noexcept {
    return op==Operation::registration?"register":op==Operation::provision?"provision":"deregister";
}
enum class DecodeResult { rejected, accepted, followup };
enum class Method { get, post, remove };
struct Request {
    struct Header {std::array<char,24> name{};std::array<char,161> value{};};
    Method method=Method::get;
    std::array<char,512> path{};
    std::array<char,1024> body{};
    std::size_t body_size=0;
    std::array<Header,2> headers{};
};
struct Metadata {
    std::array<char,13> mac{};
    std::array<char,17> attributes{};
    std::string_view lan_version="4.1.4";
    // Product build supplies its original provision query option string; no invented default.
    std::string_view provision_options;
    bool reset=false;
};
struct Response {
    std::array<char,23> did{};
    std::array<char,33> passcode{};
    std::array<char,61> host{};
    std::array<char,33> device_name{};
    std::array<char,65> device_secret{};
    std::uint16_t port=0;
    std::uint64_t timestamp=0;
    std::int32_t timezone=0;
    unsigned restart_hours=0;
    int error_code=0;
    bool has_passcode=false,has_timestamp=false,has_timezone=false,has_ecmi=false,ecmi=false,has_restart=false;
};
using Cipher=bool(*)(protocol::Bytes,const std::uint8_t*,std::uint8_t*,std::size_t,std::size_t&) noexcept;
using UpdateJson=bool(*)(std::string_view,ota::CheckReply&) noexcept;
using Json=bool(*)(Operation,std::string_view,Response&) noexcept;
bool valid_host(std::string_view) noexcept;
// Download hosts are independent of the legacy 60-byte cloud settings limit.
struct Address {std::array<char,254> host{};std::uint16_t port=0;bool tls=false;};
bool saved_address(const storage::Snapshot&,bool tls,Address&) noexcept;
inline constexpr std::size_t http_body_limit=4096;
struct HttpResponse {std::uint32_t token=0;int status=0;bool complete=false;std::array<char,http_body_limit> body{};std::size_t size=0;};
struct HttpView {
    std::uint32_t token=0;
    int status=0;
    bool complete=false;
    std::string_view body;
    // Transient upload transport failure; the owner may replay the whole POST.
    // A lost response may leave a duplicate file on the server. Cancellation,
    // exhausted deadlines and explicit HTTP/validation errors are not retryable.
    bool upload_retryable=false;
};
enum class HttpStart {rejected,retryable,started};
// Scatter/gather upload body. Caller owns immutable segments through the
// borrowed terminal response, including cancellation. Transport never copies
// the entire file; prefix/suffix can contain a provider's multipart framing.
struct UploadBody {
    protocol::Bytes prefix,file,suffix;
    std::uint32_t timeout_ms=120000;
};
class Http {
public:
    virtual ~Http()=default;
    virtual bool begin(const Address&,const Request&,std::uint32_t token) noexcept=0;
    // Owner-task only. Deliver once, after the producer has stopped touching the
    // response. The view survives until the next begin* call; consume it before
    // starting another job. False leaves output unchanged. Borrowed/streaming
    // results remain isolated from this ordinary-response path.
    virtual bool poll(HttpView&) noexcept=0;
    // Copying convenience for callers that need to retain the response.
    bool poll(HttpResponse&) noexcept;
    virtual void cancel() noexcept=0;
    // Isolated ordinary request while another business owner has borrowed the transport.
    // Its result cannot be consumed by poll(), or overwritten by another begin().
    virtual bool begin_borrowed(const Address&,const Request&,std::uint32_t) noexcept {return false;}
    virtual bool begin_upload(const Address&,const Request&,const UploadBody&,std::uint32_t) noexcept {return false;}
    // View is valid until the next begin* call; consume it synchronously on the owner task.
    virtual bool poll_borrowed(std::uint32_t,HttpView&) noexcept {return false;}
    virtual void cancel_borrowed(std::uint32_t) noexcept {}
    // False until both the I/O task and its pending result have been drained.
    // Adapters must opt in before another business owner can borrow this transport.
    virtual bool idle() const noexcept {return false;}
    // A retryable start failure leaves no in-flight request or result to drain.
    virtual HttpStart begin_upload_with_status(const Address& address,const Request& request,
        const UploadBody& body,std::uint32_t token) noexcept {
        return begin_upload(address,request,body,token)?HttpStart::started:HttpStart::rejected;
    }
};
enum class State { offline, registering, provisioning, ready, backoff, invalid_configuration, storage_failure, exhausted };
enum class RefreshState {idle,waiting,backoff};
constexpr std::uint64_t provision_refresh_ms=3600000; // Legacy PROVISION_REGET_INTERVAL.
enum class CleanupState {idle,waiting,backoff,not_applicable,invalid_identity};
enum class Preparation { invalid, request, ready, pending, retry };
// Provider owns cloud identity, wire format and persistence policy. Service owns I/O,
// deadlines, stale-response rejection, retries and exclusive HTTP borrowing.
class HttpProtocol {
public:
    virtual ~HttpProtocol()=default;
    virtual void registration_driver(mqtt::Driver&) noexcept {}
    virtual ConfigurationScope configuration_scope() const noexcept {return {};}
    virtual bool initial(const storage::Snapshot&,Operation&) const noexcept=0;
    virtual Preparation prepare(Operation,const storage::Snapshot&,const Metadata&,bool tls,Address&,Request&,Response&) noexcept=0;
    // Optional provider enrollment, polled only after HTTP has fully drained.
    virtual Preparation poll_preparation(const storage::Snapshot&,const Metadata&,bool,Address&,Request&,Response&) noexcept {return Preparation::invalid;}
    virtual void cancel_preparation() noexcept {}
    virtual bool preparation_idle() const noexcept {return true;}
    virtual bool decode(Operation,int,std::string_view,const storage::Snapshot&,Response&) noexcept=0;
    // A completed recovery step can require another request without supplying credentials.
    virtual DecodeResult decode_result(Operation op,int status,std::string_view body,const storage::Snapshot& saved,Response& out) noexcept {
        return decode(op,status,body,saved,out)?DecodeResult::accepted:DecodeResult::rejected;
    }
    virtual const char* request_name(Operation op) const noexcept {return operation_name(op);}
    virtual bool commit(Operation,runtime::Boot&,const Response&) noexcept=0;
    virtual CleanupState cleanup(const storage::Snapshot&) const noexcept {return CleanupState::not_applicable;}
    virtual std::uint64_t refresh_interval() const noexcept {return 0;}
    virtual bool ota_checks() const noexcept {return false;}
    virtual bool multi_ota_checks() const noexcept {return false;}
    virtual bool prepare_update(const storage::Snapshot&,const ota::CheckMetadata&,bool,Address&,Request&) noexcept {return false;}
    virtual bool decode_update(std::string_view,ota::CheckReply&) noexcept {return false;}
    virtual identity::Renewal renew(runtime::Boot&,identity::Random) noexcept=0;
    virtual bool file_upload_supported() const noexcept {return false;}
};
HttpProtocol& unavailable_http() noexcept;
// One worker owns this state machine; HTTP I/O may run elsewhere through a bounded mailbox.
class Service {
public:
    Service(runtime::Boot& boot,HttpProtocol& codec,Http& http,storage::Md5 md5,const Metadata& metadata,bool tls=false) noexcept
        : boot_(boot),codec_(codec),http_(http),md5_(md5),metadata_(metadata),tls_(tls) {}
    bool metadata(const Metadata& value) noexcept {if(state_!=State::offline)return false;metadata_=value;return true;}
    void start(std::uint64_t now,const char* reason="requested") noexcept;
    void stop() noexcept;
    bool drain_stopped() noexcept {if(state_!=State::offline)return false;HttpView response;http_.poll(response);return http_.idle() && codec_.preparation_idle();}
    void tick(std::uint64_t now) noexcept;
    // Worker-only exclusive loan for OTA checks and image downloads. Zero means unavailable.
    // The borrower must drain its terminal event before returning the loan.
    std::uint32_t borrow_http() noexcept;
    bool release_http(std::uint32_t loan) noexcept;
    bool owns_http(std::uint32_t loan) const noexcept {return loan && loan==http_loan_;}
    // Broker generation stays stable across background HTTP cleanup requests.
    std::uint32_t generation() const noexcept {return ready_generation_;}
    RefreshState refresh_state() const noexcept {return refresh_;}
    CleanupState cleanup_state() const noexcept {return cleanup_;}
    bool ota_checks() const noexcept {return codec_.ota_checks();}
    bool multi_ota_checks() const noexcept {return codec_.multi_ota_checks();}
    bool file_upload_supported() const noexcept {return codec_.file_upload_supported();}
    bool prepare_update(const ota::CheckMetadata& m,Address& a,Request& r) noexcept {return boot_.snapshot() && codec_.prepare_update(*boot_.snapshot(),m,tls_,a,r);}
    bool decode_update(std::string_view body,ota::CheckReply& out) noexcept {return codec_.decode_update(body,out);}
    identity::Renewal renew(identity::Random random) noexcept {return codec_.renew(boot_,random);}
    Operation operation() const noexcept {return operation_;}
    State state() const noexcept {return state_;}
    bool configuration_fingerprint(std::array<std::uint8_t,16>& out) const noexcept;
    const Response* broker() const noexcept {return state_==State::ready?&response_:nullptr;}
private:
    void complete(const Response&) noexcept;
    void issue(Operation) noexcept;
    void submit(Preparation,const Address&,const Response&) noexcept;
    void retry(bool followup=false) noexcept;
    bool unchanged() noexcept;
    runtime::Boot& boot_;HttpProtocol& codec_;Http& http_;storage::Md5 md5_;Metadata metadata_;bool tls_;
    Request request_{};Response response_{};
    std::array<std::uint8_t,16> fingerprint_{};
    mutable std::array<std::uint8_t,16> snapshot_digest_{},scoped_digest_{};
    mutable ConfigurationScope cached_scope_{};
    mutable bool scope_cache_valid_=false;
    Operation operation_=Operation::provision;
    State state_=State::offline;
    CleanupState cleanup_=CleanupState::idle;
    RefreshState refresh_=RefreshState::idle;
    std::uint64_t refresh_at_=0;
    std::uint64_t now_=0,deadline_=0;
    std::uint32_t token_=0,ready_generation_=0;
    unsigned failures_=0;
    bool preparing_=false;
    std::uint32_t http_loan_=0,last_http_loan_=0;
};

}
