#pragma once
#include "gagent/network/ble_protocol.hpp"
#if GAGENT_BLE_PROTOCOL_VERSION == 5
#include "gagent/network/ble_v5.hpp"
#endif
#include "gagent/network/local_reset.hpp"
#include "gagent/ota/ble_ota.hpp"
#include "gagent/protocol/device.hpp"
#include "gagent/storage/storage.hpp"
#include "gagent/provisioning/onboarding.hpp"
#include "gagent/provisioning/binding.hpp"
namespace gagent::local {
constexpr std::size_t client_limit=2, frame_limit=2088, send_limit=2*frame_limit;
struct Peer { std::uint32_t ipv4; std::uint16_t port; }; // network byte order address, host byte order port
struct Discovery {
    std::array<std::uint8_t,6> mac{};
    std::array<std::uint8_t,8> attributes{};
    std::array<char,33> firmware{};
    std::array<char,8> mcu_software{};
    std::array<char,16> protocol_version{};
    std::array<std::uint8_t,8> module_hardware{},module_software{};
    std::array<char,8> mcu_hardware{},business_protocol{};
};
using Encrypt=bool (*)(protocol::Bytes,const std::uint8_t*,std::uint8_t*,std::size_t,std::size_t&) noexcept;
class Transport {
public:
    virtual ~Transport()=default;
    // Nonblocking write: -1 failure, 0 would-block, positive bytes consumed.
    virtual int write(std::uint32_t session,protocol::Bytes) noexcept=0;
    virtual std::size_t packet_capacity(std::uint32_t) const noexcept {return 20;}
    virtual bool datagram(Peer,protocol::Bytes) noexcept {return false;}
    virtual bool announce(protocol::Bytes) noexcept {return false;}
    virtual void close(std::uint32_t session) noexcept=0;
};
// All entry points are called by the runtime owner. Per-session receive/send buffers are owned.
class Server : public device::Route, private protocol::Consumer {
public:
    using Allocate=void* (*)(std::size_t);
    using Release=void (*)(void*);
    Server(const Server&)=delete;
    Server& operator=(const Server&)=delete;
    bool configure(const storage::Snapshot&,const Discovery&) noexcept;
    // Refresh identity after a durable transaction; authenticated sessions lose access,
    // unauthenticated provisioning connections retain their stream and already queued ACK.
    bool refresh(const storage::Snapshot&,const Discovery&) noexcept;
    static bool fingerprint(const storage::Snapshot&,storage::Md5,std::array<std::uint8_t,16>&) noexcept;
    bool skip_login() const noexcept {return skip_login_;}
    // Product policy may change only before any session/identity is active.
    bool login_policy(bool skip) noexcept;
    std::uint32_t accept(std::uint64_t now) noexcept;
    protocol::Status receive(std::uint32_t,protocol::Bytes) noexcept;
    protocol::Status receive_datagram(Peer,protocol::Bytes) noexcept;
    protocol::Status announce(std::uint16_t command) noexcept;
    void advance(std::uint64_t now) noexcept {if(now>=now_)now_=now;}
    void tick(std::uint64_t) noexcept;
    void disconnect(std::uint32_t) noexcept;
    void stop() noexcept;
    void temporary(onboarding::TemporaryControl& control) noexcept {temporary_=&control;}
    void access_points(wifi::Driver& driver) noexcept {wifi_=&driver;}
    void binding(binding::Service& service) noexcept {binding_=&service;}
    void onboarding(onboarding::Service& service) noexcept {onboarding_=&service;}
    bool onboarding_open(std::uint64_t now) const noexcept {return onboarding_ && onboarding_->is_open(now);}
    bool pending_output() const noexcept;
    std::size_t packet_capacity() const noexcept;
    std::size_t authenticated_clients(std::uint64_t now) const noexcept;
    void network_event(std::uint8_t event) noexcept;
    bool configured() const noexcept {return configured_;}
    protocol::Status send(const protocol::Context&,const protocol::Frame&) noexcept override;
    device::Broadcast broadcast(const protocol::Frame&) noexcept override;
    void failed(std::uint32_t session) noexcept override {disconnect(session);}
    void ota(ota::BleOta& service) noexcept {if(channel_==protocol::Channel::ble)ota_=&service;}
    void resets(ResetControl& owner) noexcept {reset_=&owner;}
    void router(device::Router& router) noexcept {router_=&router;}
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    void v5_crypto(ble::v5::Hash hash,ble::v5::Crypto crypto={}) noexcept {v5_hash_=hash;v5_crypto_=crypto;}
    void v5_clock(ble::v5::SetTime fn,void* arg) noexcept {v5_time_=fn;v5_time_arg_=arg;}
#endif
protected:
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    struct V5Workspace {
        std::array<std::uint8_t,frame_limit+64> encoded{};
#if GAGENT_BLE_V5_SECURITY
        std::array<std::uint8_t,frame_limit> plain{},cipher{};
#endif
    };
    void v5_workspace(V5Workspace& value) noexcept {v5_workspace_=&value;}
#endif
    struct Session {
        std::uint8_t* rx=nullptr;
        // Storage belongs to the concrete transport, which chooses its queue size.
        struct SendBuffer {
            std::uint8_t* bytes=nullptr;
            std::size_t capacity=0;
            std::uint8_t* data() noexcept {return bytes;}
            std::size_t size() const noexcept {return capacity;}
        } tx;
        protocol::Stream stream{nullptr,0};
        std::size_t head=0,tail=0;
        std::uint32_t id=0;
        std::uint64_t activity=0,progress=0;
        bool authenticated=false,closing=false;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
        ble::v5::Assembly assembly{};
#if GAGENT_BLE_V5_SECURITY
        std::array<std::uint8_t,32> shared{};
        std::array<std::uint8_t,8> nonce_prefix{};
        std::uint32_t nonce_counter=0;
        bool secure=false;
#endif
        struct Report {std::uint32_t sequence=0;std::uint64_t sent=0;std::uint8_t command=0;};
        std::array<Report,32> reports{};
        std::uint8_t next_report=1,request=0;
#endif
    };
    // Concrete transports own exactly their supported number of sessions.
    // Only retain this view during base construction; storage is initialized by the derived class.
    Server(protocol::Channel channel,Transport& io,device::Endpoint& endpoint,Encrypt encrypt,
           Session* sessions,std::size_t count,bool skip_login=false) noexcept
        : channel_(channel),skip_login_(channel==protocol::Channel::ble && skip_login),io_(io),endpoint_(endpoint),clients_{sessions,count},encrypt_(encrypt) {}
    void session_storage(Session* sessions,std::size_t count,std::uint8_t* input,std::uint8_t* output,std::size_t capacity,
                         Allocate allocate=nullptr,Release release=nullptr) noexcept {
        clients_={sessions,count};
        allocate_=allocate;release_=release;
        for(std::size_t i=0;i<count;++i){
            auto& session=sessions[i];
            session.rx=input?input+i*frame_limit:nullptr;
            session.stream=protocol::Stream(session.rx,input?frame_limit:0);
            session.tx={output?output+i*capacity:nullptr,capacity};
        }
    }
    // Called by the concrete owner before its Session members are destroyed.
    void release_session_storage() noexcept;
private:
    void release_idle_buffers() noexcept;
    struct BufferUse {
        Server& owner;
        explicit BufferUse(Server& value) noexcept : owner(value) {++owner.buffer_users_;}
        ~BufferUse(){if(--owner.buffer_users_==0)owner.release_idle_buffers();}
    };
    Allocate allocate_=nullptr;
    Release release_=nullptr;
    unsigned buffer_users_=0;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    bool v5_load(const storage::Snapshot&,const Discovery&) noexcept;
    protocol::Status v5_receive(Session&,protocol::Bytes) noexcept;
    protocol::Status v5_send(Session&,const protocol::Frame&) noexcept;
    protocol::Status v5_queue(Session&,const protocol::Frame&) noexcept;
    protocol::Status v5_notify_reset(Session&) noexcept;
    ble::v5::Hash v5_hash_=nullptr;
    ble::v5::Crypto v5_crypto_{};
    V5Workspace* v5_workspace_=nullptr;
    ble::v5::SetTime v5_time_=nullptr;void* v5_time_arg_=nullptr;
    std::array<std::uint8_t,16> v5_key_{},v5_legacy_key_{};
#endif
    ota::BleOta* ota_=nullptr;
    ResetControl* reset_=nullptr;
    wifi::Driver* wifi_=nullptr;
    onboarding::TemporaryControl* temporary_=nullptr;
    bool load(const storage::Snapshot&,const Discovery&) noexcept;
    void clear_configuration() noexcept;
    static bool expired(const Session&,std::uint64_t now) noexcept;
    Session* find(std::uint32_t) noexcept;
    protocol::Status on_frame(const protocol::Frame&) noexcept override;
    protocol::Status login(Session&,const protocol::Frame&) noexcept;
    protocol::Status queue(Session&,protocol::Bytes) noexcept;
    protocol::Status discovery(std::size_t& size,std::uint16_t command=4) noexcept;
    void invalidate_controls() noexcept;
    void synchronize_onboarding() noexcept;
    std::uint64_t onboarding_generation_=0;
    onboarding::Service* onboarding_=nullptr;
    binding::Service* binding_=nullptr;
    Peer onboarding_peer_{};
    std::uint32_t onboarding_peer_id_=0;
    bool control_ready_=false;
    const protocol::Channel channel_;
    bool skip_login_;
    Transport& io_;
    device::Endpoint& endpoint_;
    device::Router* router_=nullptr;
    struct Sessions {
        Session* data;std::size_t size;
        Session* begin() noexcept {return data;}
        Session* end() noexcept {return size?data+size:data;}
        const Session* begin() const noexcept {return data;}
        const Session* end() const noexcept {return size?data+size:data;}
    } clients_;
    Session* processing_=nullptr;
    Encrypt encrypt_;
    std::array<std::uint8_t,16> encryption_key_{};
    std::array<std::uint8_t,256> encrypted_{};
    bool encrypted_discovery_=false;
    std::array<char,33> passcode_{},pk_{};
    std::array<char,23> did_{};
    std::size_t passcode_size_=0;
    std::size_t did_size_=0;
    std::array<std::uint8_t,256> discovery_{};
    std::size_t discovery_size_=0;
    std::array<std::uint8_t,84> device_info_{};
    std::size_t device_info_size_=0;
    std::array<std::uint8_t,frame_limit> encoded_{};
    std::array<std::uint8_t,mcu::payload_limit+4> response_{};
    std::uint64_t now_=0,last_discovery_=0;
    std::uint32_t next_id_=1;
    bool configured_=false,discovery_sent_=false;
};
}
