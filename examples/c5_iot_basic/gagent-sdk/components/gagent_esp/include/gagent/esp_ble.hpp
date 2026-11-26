#pragma once
#include "gagent/diagnostics/options.hpp"
#include "gagent/application/runtime.hpp"
#include "gagent/mailbox.hpp"
#include "host/ble_hs.h"
#include "nimble/nimble_npl.h"
#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif
namespace gagent::platform {
// Worker owns this API. SDK callbacks run only on the host task and use bounded mailboxes.
// Keep alive until busy()==false. Exclusive NimBLE ownership; no NVS erase or SDK config mutation.
class EspBle final : public ble::Radio {
public:
    void product_encoder(ble::ProductEncoder value) noexcept override {product_encoder_=value;}
    bool begin(const storage::Snapshot&,std::string_view name,const std::array<std::uint8_t,6>& station_mac,
               bool onboarding,bool skip_login=false) noexcept override;
    void poll(runtime::Engine&,std::uint64_t now) noexcept override;
    void cancel() noexcept override {cancelled_.store(true);ready_.store(false);}
    bool busy() const noexcept override {return busy_.load();}
    bool ready() const noexcept override {return ready_.load() && !cancelled_.load() && !failure_.load();}
    bool advertising() const noexcept override {return ready() && advertising_.load();}
    void onboarding(bool open) noexcept override {onboarding_.store(open);}
    void control(bool enabled) noexcept override {control_requested_.store(enabled);}
    bool control_active() const noexcept override {return ready() && control_active_.load();}
    bool beacon_capable() const noexcept override;
    bool scan(bool) noexcept override;
    beacon::State scan_state() const noexcept override;
    bool next_beacon(beacon::Report&) noexcept override;
    int write(std::uint32_t,protocol::Bytes) noexcept override;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    std::size_t packet_capacity(std::uint32_t) const noexcept override {const auto mtu=mtu_.load();return mtu<23?20:std::min<std::size_t>(255,mtu-3);}
#endif
    void close(std::uint32_t) noexcept override;
private:
    ble::ProductEncoder product_encoder_=nullptr;
    static constexpr std::size_t connections=CONFIG_BT_NIMBLE_MAX_CONNECTIONS;
    static_assert(connections==ble::client_limit,"NimBLE and BLE session capacities must match");
    static constexpr std::size_t fragment=GAGENT_BLE_PROTOCOL_VERSION==5?255:20; // Legacy notification size; valid even at the minimum ATT MTU 23.
#if defined(CONFIG_IDF_TARGET_ESP32C2) && CONFIG_IDF_TARGET_ESP32C2
    // NimBLE initializes the local MTU from this setting; negotiated writes
    // cannot exceed MTU - opcode/handle. Complete protocol frames are assembled
    // downstream, so reducing this scratch space does not limit frame length.
    static_assert(CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU>=23);
    static constexpr std::size_t rx_fragment=std::min<std::size_t>(512,CONFIG_BT_NIMBLE_ATT_PREFERRED_MTU-3);
#else
    static constexpr std::size_t rx_fragment=512;
#endif
    enum class Kind {connected,disconnected,data,sent};
    struct Event {Kind kind{};std::uint32_t token=0;std::uint16_t size=0;std::array<std::uint8_t,rx_fragment> bytes{};};
    struct Command {std::uint32_t token=0;
#if GAGENT_TRANSPORT_DIAGNOSTICS
        std::uint32_t queued_us=0;
#endif
        std::uint8_t size=0;std::array<std::uint8_t,fragment> bytes{};}; // size zero means close
    // The local server supplies complete queued frames, then retries their unconsumed suffix.
    struct Peer {std::uint32_t token=0,session=0;std::size_t frame_remaining=0;std::uint8_t pending=0;bool sent=false;std::array<std::uint8_t,fragment> bytes{};};
    struct HostPeer {std::uint32_t token=0;
#if GAGENT_TRANSPORT_DIAGNOSTICS
        std::uint32_t queued_us=0;
#endif
        std::uint16_t handle=0;bool subscribed=false,closing=false,indicating=false;std::uint8_t size=0;std::array<std::uint8_t,fragment> bytes{};};
    static void manager(void*) noexcept;
    static void host(void*) noexcept;
    static void synced() noexcept;
    static void reset(int) noexcept;
    static int gap(ble_gap_event*,void*) noexcept;
    static int access(std::uint16_t,std::uint16_t,ble_gatt_access_ctxt*,void*) noexcept;
    static void pump(ble_npl_event*) noexcept;
    void run() noexcept;
    bool configure_advertisement(const storage::Snapshot&,std::string_view,const std::array<std::uint8_t,6>&,bool,bool) noexcept;
    void configure_gatt() noexcept;
    void update_onboarding(bool) noexcept;
    void advertise() noexcept;
    void pump_scan() noexcept;
    static int discovery(ble_gap_event*,void*) noexcept;
    void pump() noexcept;
    void wake_host() noexcept;
    void wake_worker() noexcept;
#if GAGENT_TRANSPORT_DIAGNOSTICS
    void log_throughput(std::uint64_t) noexcept;
#endif
    void terminate(HostPeer&) noexcept;
    HostPeer* by_handle(std::uint16_t) noexcept;
    HostPeer* by_token(std::uint32_t) noexcept;
    bool emit(Kind,std::uint32_t,protocol::Bytes={}) noexcept;
    static std::atomic<EspBle*> owner_;
    std::array<Peer,connections> peers_{};
    std::array<HostPeer,connections> host_peers_{};
#if defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER) && CONFIG_BT_NIMBLE_ROLE_OBSERVER
    struct ScanReport {std::uint32_t revision=0;beacon::Report report{};};
    Mailbox<ScanReport,17> scan_reports_;
#endif
    std::atomic<std::uint32_t> scan_revision_{0},scan_applied_{0};
    std::atomic<bool> scan_requested_{false},scan_running_{false},scan_failed_{false},control_requested_{true},control_active_{false};
    bool scan_cancelling_=false;
    Mailbox<Event,9> events_;
    Mailbox<Command,5> commands_;
    Event incoming_{}; // Worker scratch; not shared with host producer.
    // Notification index 0 is a shared wake hint (also used by HTTP), never
    // an event payload. Worker drains BLE before exiting, keeping this live.
    std::atomic<void*> worker_task_{nullptr};
#if GAGENT_TRANSPORT_DIAGNOSTICS
    std::atomic<std::uint32_t> tx_bytes_{0},tx_packets_{0},stall_mbuf_{0},stall_reserve_{0},stall_sdk_{0};
    std::atomic<std::uint32_t> notify_us_{0},notify_max_us_{0};
    std::atomic<std::uint32_t> host_us_{0},host_max_us_{0};
    std::uint64_t stats_since_=0,last_poll_=0,poll_gap_=0;
    std::uint32_t rx_bytes_=0,rx_packets_=0;
#endif
    std::array<std::uint8_t,31> adv_{};
    std::array<std::uint8_t,25> scan_{}; // Up to 20 ASCII Aliyun PK bytes plus AD/status fields.
    std::array<char,32> name_{};
    std::uint8_t adv_size_=0,scan_size_=0,address_type_=0,status_=0;
    std::uint16_t value_handle_=0;
    std::uint32_t next_token_=1;
#if GAGENT_BLE_PROTOCOL_VERSION == 5
    std::atomic<std::uint16_t> mtu_{23};
    std::array<std::uint16_t,5> handles_{};
    std::array<std::uint8_t,94> information_{};
    std::size_t information_size_=0;
    ble_gatt_chr_def characteristics_[6]{};
#else
    ble_gatt_chr_def characteristics_[2]{};
#endif
    ble_gatt_svc_def services_[2]{};
    ble_npl_callout timer_{};
    ble_npl_event work_{};
    // Protect an external producer against manager-side NPL teardown. Worker
    // never waits for this gate; the periodic timer remains the retry fallback.
    std::atomic_flag wake_gate_=ATOMIC_FLAG_INIT;
    std::atomic<bool> wake_enabled_{false};
    bool scan_open_=false;
    std::atomic<bool> busy_{false},cancelled_{false},ready_{false},failure_{false},advertising_{false},host_exited_{true},onboarding_{false};
};
}
