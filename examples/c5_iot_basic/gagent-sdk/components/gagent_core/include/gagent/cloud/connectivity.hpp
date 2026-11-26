#pragma once
#include "gagent/application/runtime.hpp"
#include "gagent/cloud/mqtt.hpp"
#include "gagent/cloud/time.hpp"
#include "gagent/cloud/credential_recovery.hpp"
namespace gagent::runtime {
// Platform network owner calls this on its existing runtime task, never in Wi-Fi callbacks.
class NetworkServices {
public:
    virtual ~NetworkServices()=default;
    virtual void connected(std::uint64_t now) noexcept=0;
    virtual void disconnected() noexcept=0;
    virtual void tick(std::uint64_t now) noexcept=0;
    virtual void station(wifi::State) noexcept {}
    virtual void station_failure(std::uint64_t,unsigned,bool) noexcept {}
    virtual wifi::RetryPolicy retry_policy() const noexcept {return {};}
    virtual bool quiescing() const noexcept {return false;}
    virtual bool current_discovery(lan::Discovery&) const noexcept {return false;}
};
enum class RecoveryState {idle,waiting,pending_cleanup,registering,invalid_identity,entropy_failure,persistence_failure};
class Connectivity final : public NetworkServices, public time::Source {
public:
    Connectivity(Boot& boot,Engine& engine,cloud::Service& cloud,mqtt::Service& mqtt,storage::Md5 md5,const lan::Discovery& discovery,bool mqtt_tls=false,identity::Random random=nullptr) noexcept
        : boot_(boot),engine_(engine),cloud_(cloud),mqtt_(mqtt),md5_(md5),discovery_(discovery),mqtt_tls_(mqtt_tls),random_(random) {}
    bool discovery(const lan::Discovery& value) noexcept {if(connected_)return false;discovery_=value;return true;}
    void connected(std::uint64_t now) noexcept override;
    void disconnected() noexcept override;
    void tick(std::uint64_t now) noexcept override;
    bool reconnect_cloud(bool provision,std::uint64_t now) noexcept;
    RecoveryState recovery_state() const noexcept {return recovery_;}
    bool online() const noexcept {return connected_ && cloud_.state()==cloud::State::ready && mqtt_.state()==mqtt::State::ready;}
    std::uint64_t revision() const noexcept override {return clock_revision_;}
    bool unix_time(std::uint64_t now,std::uint64_t& seconds) const noexcept override;
    // Authenticated local clock input uses the same monotonic time owner as cloud sync.
    void local_time(std::uint32_t seconds,std::uint64_t now) noexcept {
        if(now<now_)return;
        unix_time_=seconds;time_set_at_=now;time_valid_=true;
        if(!++clock_revision_)++clock_revision_;
    }
private:
    bool fingerprint(std::array<std::uint8_t,16>&) noexcept;
    bool refresh_local(bool force=false) noexcept;
    void refresh_timezone(bool notify) noexcept;
    Boot& boot_;Engine& engine_;cloud::Service& cloud_;mqtt::Service& mqtt_;storage::Md5 md5_;lan::Discovery discovery_;
    std::array<std::uint8_t,16> fingerprint_{},local_fingerprint_{};
    std::array<char,20> timezone_{};
    bool local_fingerprint_valid_=false;
    std::uint32_t activated_=0;
    std::uint64_t mqtt_clock_revision_=0,clock_revision_=0,now_=0,time_set_at_=0,unix_time_=0,change_deadline_=0;
    bool connected_=false,mqtt_tls_=false,time_valid_=false;
    identity::Random random_=nullptr;
    RecoveryState recovery_=RecoveryState::idle;
    std::uint64_t recovery_due_=0,stable_since_=0;
    unsigned recovery_failures_=0;
};
}
