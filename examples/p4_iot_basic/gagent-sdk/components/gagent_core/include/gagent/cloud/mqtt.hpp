#pragma once
#include "gagent/cloud/transport_limits.hpp"
#include "gagent/cloud/channel_telemetry.hpp"
#include "gagent/cloud/mqtt_buffer.hpp"
#include "gagent/cloud/mqtt_payload.hpp"
#include <cstring>
#include "gagent/diagnostics/runtime_logs.hpp"
#include "gagent/cloud/cloud.hpp"
#include "gagent/protocol/device.hpp"
#include "gagent/ota/ota_check.hpp"
#include "gagent/ota/mcu_ota.hpp"
namespace gagent::mqtt {
constexpr std::size_t frame_limit=2088,wire_limit=GAGENT_MQTT_WIRE_LIMIT,topic_limit=GAGENT_MQTT_TOPIC_LIMIT;
constexpr std::size_t peer_limit=GAGENT_MQTT_PEER_LIMIT,response_limit=GAGENT_MQTT_RESPONSE_LIMIT;
// Each peer can have three outstanding controls. Two shared overflow records
// preserve the existing peer capacity without tripling every reply cache.
constexpr std::size_t controls_per_peer=3,control_slot_limit=peer_limit+controls_per_peer-1;
using Payload=PayloadBuffer<wire_limit>;
struct Configuration {
    cloud::Address broker{};
    std::array<char,23> did{};
    std::array<char,33> passcode{};
    std::array<char,topic_limit> server_topic{},control_topic{};
    std::array<char,GAGENT_MQTT_CLIENT_ID_SIZE> client_id{};
    std::array<char,GAGENT_MQTT_USERNAME_SIZE> username{};
    std::array<char,GAGENT_MQTT_PASSWORD_SIZE> password{};
    std::array<char,GAGENT_MQTT_PRODUCT_KEY_SIZE> product_key{};
    std::array<char,GAGENT_MQTT_DEVICE_NAME_SIZE> device_name{};
    bool v311=false,renew_on_not_authorized=false;
    std::uint8_t subscription_qos=0;
    std::array<std::array<char,topic_limit>,GAGENT_MQTT_EXTRA_TOPICS> extra_topics{};
    std::uint8_t extra_count=0;
    std::uint16_t keepalive=120;
    bool persistent=false;
    // Enrollment probe: observe CONNACK without subscribing or handling business data.
    bool connect_only=false;
};
// authentication_failed is specifically legacy CONNACK 0x04. Other refusals reconnect.
enum class EventKind { ready, data, disconnected, authentication_failed };
struct Event {
    EventKind kind=EventKind::disconnected;
    std::uint32_t epoch=0;
    std::array<char,topic_limit> topic{};
    Payload bytes;
    std::size_t size=0;
    bool retained=false;
    // Diagnostic/test copy with explicit allocation failure. Production passes leases.
    bool copy_from(const Event& other) noexcept {
        if(this==&other)return true;
        if(other.size>other.bytes.size() || !bytes.reserve(other.size))return false;
        if(other.size)std::memcpy(bytes.data(),other.bytes.data(),other.size);
        kind=other.kind;epoch=other.epoch;topic=other.topic;size=other.size;retained=other.retained;return true;
    }
};
struct RxLease {
    BufferLease<Event> storage;
    EventKind kind=EventKind::disconnected;
    std::uint32_t epoch=0;
    void reset() noexcept {storage.reset();kind=EventKind::disconnected;epoch=0;}
};
struct Publish {
    std::array<char,topic_limit> topic{};
    Payload bytes;
    std::size_t size=0;
    std::uint32_t epoch=0;
    unsigned qos=0;
    bool reset=false;
};
using TxLease=BufferLease<Publish>;
enum class ResetStatus { idle, pending, acknowledged, failed };
enum class ConnectionState {pending,ready,failed};
class Driver {
public:
    virtual ~Driver()=default;
    // All calls must return promptly; I/O and teardown belong to the platform task.
    virtual bool begin(const Configuration&,std::uint32_t epoch) noexcept=0;
    virtual bool poll(RxLease&) noexcept=0;
    // One worker reservation at a time; submit consumes it only on success.
    virtual bool reserve(TxLease&,std::size_t capacity=512) noexcept=0;
    virtual bool submit(TxLease&) noexcept=0;
    virtual bool publish(std::string_view topic,protocol::Bytes) noexcept=0;
    virtual bool publish(std::string_view topic,protocol::Bytes bytes,unsigned qos) noexcept {return qos==0 && publish(topic,bytes);}
    // One tracked reset per connection. Status must remain readable without polling business events.
    virtual bool publish_reset(std::string_view topic,protocol::Bytes bytes,unsigned qos) noexcept {return publish(topic,bytes,qos);}
    virtual ResetStatus reset_status() const noexcept {return ResetStatus::idle;}
    virtual void cancel() noexcept=0;
    // Asynchronous adapters remain busy until the cancelled I/O task has exited.
    virtual bool busy() const noexcept {return false;}
    virtual bool idle() const noexcept {return false;}
    virtual ConnectionState connection_state() const noexcept {return ConnectionState::failed;}
};
struct Update {ota::Target target=ota::Target::module;ota::Location location{};ota::Offer image{};std::array<char,33> version{};};
struct Metadata {std::array<char,33> module_version{},mcu_version{};};
enum class MessageKind { control, management, ignored, clock, update };
struct Message {
    MessageKind kind=MessageKind::control;
    protocol::Frame frame{};
    // Only one decoded result is live, and all views expire with the RX lease.
#if GAGENT_MQTT_EXTENDED_MESSAGES
    union {Update update{};std::uint64_t unix_seconds;};
#endif
    std::array<char,24> peer{};
};
class Protocol {
public:
    virtual ~Protocol()=default;
    virtual bool configure(const storage::Snapshot&,const cloud::Response&,bool,Configuration&) const noexcept=0;
    // May consume RX bytes; all returned views expire when decode/dispatch ends.
    virtual protocol::Status decode(const Configuration&,Event&,Message&,std::uint64_t now) noexcept=0;
    virtual bool topic(const Configuration&,std::string_view peer,char*,std::size_t) const noexcept=0;
    virtual protocol::Status encode(std::string_view peer,const protocol::Frame&,std::uint8_t*,std::size_t,std::size_t&) noexcept=0;
    // Safe encoding bound including terminators; zero means invalid/over limit.
    virtual std::size_t encode_capacity(std::string_view,const protocol::Frame&) const noexcept {return wire_limit;}
    virtual bool reset(const Configuration&,char*,std::size_t,std::uint8_t*,std::size_t,std::size_t&) noexcept=0;
    virtual unsigned reset_qos() const noexcept {return 0;}
    virtual void reset_accepted() noexcept {}
    // Actual App sessions only; a connected broker is not an online client.
    virtual std::uint16_t online_clients() const noexcept {return 0;}
    // Some providers acknowledge reports themselves, independently of App presence.
    virtual bool reports_acknowledged_by_cloud() const noexcept {return false;}
    virtual c0::Policy channel_policy() const noexcept {return {};}
    // unsupported means ordinary business traffic; malformed C0 must never reach MCU.
    virtual protocol::Status channel_request(const protocol::Frame&,c0::Request&) const noexcept {return protocol::Status::unsupported;}
    virtual bool encode_channel(const Configuration&,const c0::Sample&,const c0::Request&,Publish&) noexcept {return false;}
    virtual void channel_accepted() noexcept {}
    virtual void connected(std::uint64_t) noexcept {}
    virtual void metadata(const Metadata&) noexcept {}
    virtual void tick(Driver&,const Configuration&,std::uint64_t) noexcept {}
    // Encoding has no accepted-publication side effects until this callback.
    virtual void encoded_accepted(std::string_view,const protocol::Frame&) noexcept {}
    virtual std::size_t control_peers() const noexcept {return peer_limit;}
    virtual bool reports_response() const noexcept {return false;}
    virtual std::string_view log_device_id(const Configuration& config) const noexcept {return config.did.data();}
    virtual void management_reply(Driver&,const Configuration&,const Message&,protocol::Status) noexcept {}
};
Protocol& unavailable_protocol() noexcept;
class Management {
public:
    virtual ~Management()=default;
    virtual protocol::Status receive(const protocol::Frame&,std::uint64_t now) noexcept=0;
    virtual protocol::Status update(const Update&,std::uint64_t) noexcept {return protocol::Status::unsupported;}
    virtual protocol::Status channel_telemetry(const c0::Request&,std::uint64_t) noexcept {return protocol::Status::unsupported;}
    virtual protocol::Status runtime_log(bool,std::string_view,std::uint64_t) noexcept {return protocol::Status::unsupported;}
};
enum class State { offline, connecting, ready, backoff, authentication_failed, invalid_configuration, exhausted };
class Service final : public device::Route {
public:
    Service(Driver& driver,device::Router& router,storage::Md5 md5,Protocol& protocol,
            Payload::Reallocate response_allocate=std::realloc) noexcept
        : driver_(driver),router_(router),md5_(md5),protocol_(protocol),response_allocate_(response_allocate) {}
    bool start(const storage::Snapshot&,const cloud::Response&,std::uint64_t now,bool tls=false) noexcept;
    bool matches(const storage::Snapshot&,const cloud::Response&,bool tls=false) const noexcept;
    void stop() noexcept;
    void tick(std::uint64_t now) noexcept;
    void management(Management& target) noexcept {management_=&target;}
    void metadata(const Metadata& value) noexcept {protocol_.metadata(value);}
    std::uint64_t clock_revision() const noexcept {return clock_revision_;}
    bool unix_time(std::uint64_t now,std::uint64_t& seconds) const noexcept {
        if(!clock_revision_ || now<clock_set_at_)return false;
        seconds=clock_seconds_+(now-clock_set_at_)/1000;return true;
    }
    std::uint16_t remote_clients() const noexcept {return remote_clients_;}
    State state() const noexcept {return state_;}
    c0::Policy channel_policy() const noexcept {return protocol_.channel_policy();}
    bool report_channel(const c0::Sample&,const c0::Request& request={}) noexcept;
    std::uint32_t connection_epoch() const noexcept {return epoch_;}
    std::size_t reply_cache_bytes() const noexcept {
        std::size_t total=0;for(const auto& peer:peers_)total+=peer.response.size();return total;
    }
    protocol::Status send(const protocol::Context&,const protocol::Frame&) noexcept override;
    void failed(std::uint32_t session) noexcept override;
    device::Broadcast broadcast(const protocol::Frame&) noexcept override;
    bool report(const protocol::Frame&) noexcept;
    // Provider owns QoS and any durable retry intent; this only queues the live notice.
    bool publish_reset() noexcept;
    ResetStatus reset_status() const noexcept {return driver_.reset_status();}
private:
    struct Peer {
        std::array<char,24> uid{};
        std::array<std::uint8_t,16> digest{};
#ifdef GAGENT_C2_MEMORY_PROFILE
        PayloadBuffer<response_limit> response;
#else
        std::array<std::uint8_t,response_limit> response{};
#endif
        std::size_t response_size=0;
        std::uint32_t session=0,sequence=0;
        std::uint64_t activity=0;
        bool sequenced=false,pending=false,remembered=false;
    };
    void connect() noexcept;
    void down() noexcept;
    void retry() noexcept;
    protocol::Status receive(Event&) noexcept;
    Peer* find(std::uint32_t) noexcept;
    bool publish(std::string_view uid,protocol::Bytes) noexcept;
    Driver& driver_;device::Router& router_;storage::Md5 md5_;Management* management_=nullptr;
    Protocol& protocol_;
    [[maybe_unused]] Payload::Reallocate response_allocate_;
    Configuration configuration_{};Message message_{};
    std::array<Peer,control_slot_limit> peers_{};
    std::array<char,topic_limit> topic_{};
    State state_=State::offline;
    std::uint32_t epoch_=0,next_session_=1;
    std::uint64_t now_=0,deadline_=0,online_since_=0;
    std::uint64_t clock_revision_=0,clock_seconds_=0,clock_set_at_=0;
    unsigned failures_=0;
    bool waiting_driver_=false;
    std::uint16_t remote_clients_=0;
};
enum class Assembly { incomplete, complete, invalid };
// SDK MQTT_EVENT_DATA can split a single publish; never dispatch a prefix or mix messages.
// Caller reserves total payload capacity before the first fragment; assembler never allocates.
class Assembler {
public:
    Assembly append_fragment(Event&,std::string_view topic,protocol::Bytes,std::size_t offset,std::size_t total,bool retained) noexcept;
    bool append(Event& e,std::string_view topic,protocol::Bytes bytes,std::size_t offset,std::size_t total,bool retained) noexcept {
        return append_fragment(e,topic,bytes,offset,total,retained)==Assembly::complete;
    }
    void reset() noexcept {used_=expected_=0;}
private:
    std::size_t used_=0,expected_=0;
};
}
