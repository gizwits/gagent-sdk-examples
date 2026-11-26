#pragma once
#include "gagent/storage/storage.hpp"
namespace gagent::factory {class Scanner;}
namespace gagent::wifi {
// Association and DHCP have separate budgets; onboarding remains bounded.
inline constexpr std::uint32_t dhcp_timeout_ms=30000;
inline constexpr std::uint32_t dhcp_recovery_timeout_ms=60000;
inline constexpr unsigned dhcp_short_attempts=2;
inline constexpr std::uint32_t onboarding_timeout_ms=120000;
enum class Band : std::uint8_t {unknown=0,ghz2=2,ghz5=5};
Band channel_band(std::uint8_t) noexcept;
enum class SavedScan {full,fast,preferred_band,other_band};
// Decoding is legacy-specific: only <urlcrypto> values interpret %HH; '+' is literal.
bool decode_saved(std::string_view, std::uint8_t*, std::size_t capacity, std::size_t& size) noexcept;
struct Credential {
    std::array<std::uint8_t,32> ssid{};
    std::array<std::uint8_t,64> password{};
    std::array<std::uint8_t,6> bssid{};
    std::size_t ssid_size=0,password_size=0;
    bool bssid_set=false;
    // Optional acceleration metadata; never part of credential identity or an AP lock.
    Band band=Band::unknown;
    std::uint8_t channel=0;
};
// One validation/encoding contract for saved Wi-Fi and every onboarding channel.
bool valid(const Credential&) noexcept;
bool same(const Credential&,const Credential&) noexcept;
bool encode_saved(std::string_view, char*, std::size_t capacity, std::size_t& size) noexcept;
enum class CredentialStatus { missing, valid, invalid };
struct Saved {
    std::array<Credential,2> credentials{};
    std::array<CredentialStatus,2> status{CredentialStatus::missing,CredentialStatus::missing};
};
Saved read_saved(const storage::Snapshot&) noexcept;
enum class Link { connecting, online, failed };
struct Observation {
    bool known=false,station=false,ap=false,connected=false,signal_known=false;
    std::int8_t rssi=0;
    std::array<std::uint8_t,4> ipv4{}; // Network byte order, four octets rather than a host integer.
    std::uint8_t channel=0;
};
// Prepared before either interface starts. These are board parameters, never saved STA credentials.
struct AccessPoint {
    Credential credential{}; // BSSID selection is not applicable; passwords are empty or 8–63 bytes.
    std::array<std::uint8_t,4> address{10,10,100,254}; // Legacy /24 subnet, also the gateway.
    std::uint8_t channel=1,max_clients=4;
};
bool valid(const AccessPoint&) noexcept;
enum class AccessPointState { stopped, starting, online, stopping, failed };
class AccessPointDriver {
public:
    virtual ~AccessPointDriver()=default;
    // Same Worker and radio owner as Driver. Never change AP identity on a running radio.
    virtual bool prepare_ap(const AccessPoint&) noexcept=0;
    virtual bool start_ap() noexcept=0;
    virtual AccessPointState poll_ap() noexcept=0;
    virtual AccessPointState ap_state() const noexcept=0;
    virtual bool end_ap() noexcept=0;
};
enum class CaptureRadioState {stopped,starting,ready,stopping,failed};
class CaptureRadio {
public:
    virtual ~CaptureRadio()=default;
    // The station manager and AP must be stopped before entering this exclusive radio mode.
    virtual bool start_capture() noexcept=0;
    virtual CaptureRadioState poll_capture() noexcept=0;
    // Caller must first prove the decoder has stopped and drained its callbacks.
    virtual bool stop_capture() noexcept=0;
};
// Legacy AP queries expose the latest completed scan, in SDK RSSI order.
struct ScanEntry {std::array<std::uint8_t,32> ssid{};std::uint8_t size=0;std::int8_t rssi=0;std::array<std::uint8_t,6> bssid{};std::uint8_t channel=0;};
struct ScanList {std::array<ScanEntry,32> entries{};std::size_t count=0;};
// Returns a borrowed entry using legacy five-stage matching; never changes credentials.
const ScanEntry* choose_access_point(const Credential&,const ScanList&) noexcept;
// Streaming bounded selection: retain the best legacy match class/distance,
// then strongest RSSI first. Only real scanned records can become candidates.
void add_access_point_candidate(const Credential&,const ScanEntry&,ScanList&) noexcept;
enum class ScanState {idle,scanning,ready,stopping,failed,quarantined};
class Survey {
public:
    virtual ~Survey()=default;
    // Exclusive radio phase. Caller releases STA/AP/capture first; a failed start
    // can require stop/drain. Only idle proves that another cycle may start.
    virtual bool start_scan(std::uint64_t now) noexcept=0;
    virtual ScanState poll_scan(std::uint64_t now) noexcept=0;
    virtual ScanState scan_state() const noexcept=0;
    virtual void stop_scan(std::uint64_t now) noexcept=0;
    virtual const ScanList& scan_results() const noexcept=0;
};
class Driver {
public:
    virtual ~Driver()=default;
    // Must copy input. begin/end/poll run only on the runtime worker.
    virtual bool begin(const Credential&) noexcept=0;
    // Unrestricted recovery prefers the strongest matching AP across all channels.
    // Drivers without separate scan policies retain their existing connection behavior.
    virtual bool begin_saved(const Credential& credential) noexcept {return begin(credential);}
    virtual bool saved_hints() const noexcept {return false;}
    virtual bool dual_band() const noexcept {return false;}
    virtual bool begin_saved_scan(const Credential& credential,SavedScan,std::uint64_t) noexcept {return begin_saved(credential);}
    virtual Link poll_saved(std::uint64_t) noexcept {return poll();}
    virtual bool selects_onboarding_ap() const noexcept {return false;}
    virtual bool begin_onboarding(const Credential& credential,std::uint64_t) noexcept {return begin(credential);}
    virtual Link poll_onboarding(std::uint64_t) noexcept {return poll();}
    // Logical credentials after selection: actual SSID, original password/BSSID hint.
    // The temporary AP lock must never be returned here or persisted.
    virtual const Credential* onboarding_credential() const noexcept {return nullptr;}
    virtual Link poll() noexcept=0;
    // Current attempt is associated but still awaiting its first usable IP.
    virtual bool waiting_for_ip() const noexcept {return false;}
    // Absolute worker-clock deadline for the current associated attempt; zero if unavailable.
    virtual std::uint64_t ip_wait_deadline() const noexcept {return 0;}
    // Cancel the current attempt. poll must never report its stale online event later.
    virtual void end() noexcept=0;
    // Cached, read-only snapshot on the same worker as poll(). Never calls application code or the SDK.
    virtual Observation observation() const noexcept {return {};}
    // Worker-only borrowed cache. Reading never starts a scan or changes radio mode.
    virtual const ScanList* access_points() const noexcept {return nullptr;}
    // Optional background discovery while waiting for SoftAP/BLE credentials.
    virtual void onboarding_scan(bool,std::uint64_t) noexcept {}
    virtual void shutdown() noexcept {end();}
    // True only after STA/AP/capture teardown has fully released the radio.
    virtual bool radio_idle() const noexcept {return false;}
    // Optional AP capability of this same driver/owner, never a separate Wi-Fi instance.
    virtual AccessPointDriver* access_point() noexcept {return nullptr;}
    virtual CaptureRadio* capture_radio() noexcept {return nullptr;}
    virtual Survey* survey() noexcept {return nullptr;}
    virtual factory::Scanner* scanner() noexcept {return nullptr;}
};
struct RetryPolicy {std::uint32_t attempt_timeout_ms=30000;bool alternate_saved=true;};
class Network {
public:
    virtual ~Network()=default;
    virtual bool up(std::uint64_t now) noexcept=0;
    virtual void down() noexcept=0;
    virtual void failure(std::uint64_t,unsigned,bool) noexcept {}
    virtual RetryPolicy retry_policy() const noexcept {return {};}
};
enum class State { stopped, no_credentials, connecting, online, backoff, failed };
// Owns reconnection policy, independently of transports/ESP events.
class Manager {
public:
    Manager(Driver& driver,Network& network) noexcept : driver_(driver),network_(network) {}
    bool start(const storage::Snapshot&,std::uint64_t now) noexcept;
    bool start_onboarding(const storage::Snapshot&,std::uint64_t now) noexcept;
    // Adopt a verified trial connection after its credentials were durably saved.
    bool adopt(const storage::Snapshot&,const Credential&,std::uint64_t now) noexcept;
    void tick(std::uint64_t now) noexcept;
    void stop() noexcept;
    State state() const noexcept {return state_;}
    unsigned slot() const noexcept {return slot_;}
    bool matches(const storage::Snapshot&) const noexcept;
    bool retry_due(std::uint64_t now) const noexcept {return state_==State::backoff && now>=now_ && now>=deadline_;}
    // Resume the existing retry after exclusive scanning; retain its failure budget.
    bool retry_with(const Credential&,std::uint64_t now) noexcept;
    // Accept only the durable current-SSID update; never changes the live connection.
    bool accept_current_ssid(const storage::Snapshot&) noexcept;
    // Best-effort optional metadata, once per connection; failure never tears down Wi-Fi.
    void remember_channel(runtime::Boot&) noexcept;
    const Credential& current() const noexcept {return current_;}
    const Saved& saved() const noexcept {return saved_;}
private:
    bool start(const storage::Snapshot&,std::uint64_t now,bool onboarding) noexcept;
    void attempt() noexcept;
    void retry() noexcept;
    bool advance_scan() noexcept;
    Driver& driver_;Network& network_;
    Saved saved_{};
    Credential current_{};
    State state_=State::stopped;
    unsigned slot_=0,failures_=0,consecutive_failures_=0;
    std::uint64_t now_=0,deadline_=0,online_since_=0;
    bool attempt_bounded_=true,onboarding_=false;
    SavedScan scan_=SavedScan::full;
    bool hint_written_=false;
};
}
