#pragma once
#include "gagent/diagnostics/options.hpp"
#include "gagent/application/runtime.hpp"
#include "gagent/application/startup.hpp"
#include "gagent/application/preparation.hpp"
#include "gagent/application/mcu_bootstrap.hpp"
#include "gagent/network/bluetooth.hpp"
#include "gagent/provisioning/softap.hpp"
#include "gagent/provisioning/wireless_onboarding.hpp"
#include "gagent/network/wifi.hpp"
#include "gagent/cloud/connectivity.hpp"
#include "gagent/serial_extension.hpp"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
namespace gagent::platform {
// lwIP and official UART APIs; all I/O is nonblocking and owned by one worker.
class EspIo final : public lan::Transport, public device::Serial {
public:
    bool open(bool network_ready) noexcept;
    bool configure_uart(uart_port_t,int tx_pin,int rx_pin,int baud) noexcept;
    // Configure before UART installation/Worker startup; nullptr preserves the default path.
    bool serial_extension(SerialExtension* value) noexcept {
        if(uart_installed_)return false;
        extension_=value;return true;
    }
    void poll(runtime::Engine&,std::uint64_t now) noexcept;
    void close_network() noexcept;
    bool multicast(bool enabled) noexcept;
    // Retain sessions only on currently usable local interfaces; transport routing, not protocol logic.
    bool routes(runtime::Engine&,const std::array<std::uint8_t,4>&,const std::array<std::uint8_t,4>& = {}) noexcept;
    void shutdown() noexcept;
    int write(std::uint32_t,protocol::Bytes) noexcept override;
    bool datagram(lan::Peer,protocol::Bytes) noexcept override;
    bool announce(protocol::Bytes) noexcept override;
    void close(std::uint32_t) noexcept override;
    bool send(protocol::Bytes) noexcept override;
    void clear() noexcept override;
    bool idle() const noexcept override;
    std::uint32_t baudrate() const noexcept override {return uart_installed_?uart_baud_:0;}
    bool reconfigure_baud(std::uint32_t) noexcept override;
private:
    struct Connection {
        int fd=-1;std::uint32_t id=0;std::array<std::uint8_t,4> address{};
#if GAGENT_TRANSPORT_DIAGNOSTICS
        std::uint64_t stalled_since_ms=0,last_stall_log_ms=0;
#endif
    };
    bool allowed(const std::array<std::uint8_t,4>&) const noexcept;
    std::array<std::uint8_t,4> primary_address_{},secondary_address_{};
    bool filter_addresses_=false,multicast_joined_=false;
    std::array<Connection,lan::client_limit> clients_{};
    std::array<std::uint8_t,runtime::event_bytes> receive_{};
    std::array<std::uint8_t,mcu::wire_limit> serial_tx_{};
    std::size_t serial_offset_=0,serial_size_=0;
    int listener_=-1,udp_=-1,announcement_=-1;
#if GAGENT_TRANSPORT_DIAGNOSTICS
    // Worker-owned counters; packet contents and provisioning credentials are never logged.
    struct UdpDiagnostics {
        std::uint64_t since_ms=0;
        unsigned rx=0,discovery_rx=0,tx=0,tx_failed=0,announce_tx=0,announce_failed=0;
        std::uint32_t peer_ipv4=0;
        std::uint16_t peer_port=0;
        int last_errno=0;
    } udp_diagnostics_{};
#endif
    uart_port_t uart_=UART_NUM_0;
    bool uart_installed_=false;
    std::uint32_t uart_baud_=0;
    SerialExtension* extension_=nullptr;
};
// Engine/EspIo and this object must outlive the task. Never call Engine from another task.
// UART (if required) is configured before start; request_stop is the only cross-task mutation.
enum class PreparationState {not_requested,pending,ready,failed,cancelled};
class Worker : private wifi::Network {
public:
    Worker(runtime::Engine& engine,EspIo& io) noexcept : engine_(engine),io_(io) {}
    bool start(const lan::Discovery&,bool network_ready,bool allow_migration=false) noexcept;
    bool start_wifi(const lan::Discovery&,runtime::Boot&,wifi::Driver&,bool allow_migration=false) noexcept;
    bool services(runtime::NetworkServices& services) noexcept {if(running_.load())return false;services_=&services;return true;}
    bool bluetooth(runtime::Bluetooth& service) noexcept {if(running_.load() || !service.attach())return false;bluetooth_=&service;return true;}
    bool softap(runtime::SoftAp& service) noexcept {if(running_.load() || wireless_ || !engine_.onboarding(service.onboarding()))return false;softap_=&service;service.onboarding().modes(service);return true;}
    bool wireless(runtime::WirelessOnboarding& service) noexcept {
        if(running_.load() || (softap_ && softap_!=service.softap()) || !engine_.onboarding(service.onboarding()))return false;
        wireless_=&service;softap_=service.softap();service.onboarding().modes(service);return true;
    }
    // Explicit startup policy, run on this Worker's stack before any services activate.
    // Defaults are copied as views and must outlive the Worker. nullptr uses saved product data only.
    bool startup(const runtime::StartupDefaults* defaults,identity::Random random) noexcept {
        if(running_.load() || bootstrap_)return false;
        startup_defaults_=defaults?*defaults:runtime::StartupDefaults{};
        startup_has_defaults_=defaults!=nullptr;startup_random_=random;startup_configured_=true;
        startup_state_.store(runtime::StartupState::pending);return true;
    }
    bool mcu_startup(runtime::McuBootstrap& bootstrap) noexcept {
        if(running_.load() || startup_configured_ || &bootstrap.boot()!=&engine_.boot() || !engine_.owns(bootstrap.mcu()))return false;
        bootstrap_=&bootstrap;startup_state_.store(runtime::StartupState::pending);return true;
    }
    bool preparation(runtime::Preparation& value) noexcept {
        if(running_.load())return false;
        preparation_=&value;preparation_state_.store(PreparationState::pending);return true;
    }
    PreparationState preparation_state() const noexcept {return preparation_state_.load();}
    runtime::StartupState startup_state() const noexcept {return startup_state_.load();}
    void request_stop() noexcept {stop_.store(true);}
    bool running() const noexcept {return running_.load();}
    bool maintenance(void (*fn)(void*,std::uint64_t,bool) noexcept,void* context) noexcept {
        if(running_.load())return false;
        maintenance_=fn;maintenance_context_=context;return true;
    }
    // Platform idle hook; must return within ticks and may wake early for I/O.
    bool wait_hook(void (*fn)(void*,unsigned) noexcept,void* context) noexcept {
        if(running_.load())return false;
        wait_=fn;wait_context_=context;return true;
    }
private:
    static void entry(void*) noexcept;
    bool up(std::uint64_t now) noexcept override;
    void down() noexcept override;
    wifi::RetryPolicy retry_policy() const noexcept override {return services_?services_->retry_policy():wifi::RetryPolicy{};}
    void failure(std::uint64_t now,unsigned count,bool lost) noexcept override {if(services_)services_->station_failure(now,count,lost);}
    bool local_network(std::uint64_t now) noexcept;
    void close_network() noexcept;
    runtime::Engine& engine_;
    EspIo& io_;
    void (*maintenance_)(void*,std::uint64_t,bool) noexcept=nullptr;
    void* maintenance_context_=nullptr;
    void (*wait_)(void*,unsigned) noexcept=nullptr;
    void* wait_context_=nullptr;
    lan::Discovery discovery_{};
    std::atomic<bool> running_{false},stop_{false};
    runtime::Boot* boot_=nullptr;
    wifi::Driver* wifi_=nullptr;
    runtime::NetworkServices* services_=nullptr;
    runtime::Bluetooth* bluetooth_=nullptr;
    runtime::SoftAp* softap_=nullptr;
    runtime::WirelessOnboarding* wireless_=nullptr;
    bool station_online_=false,sockets_open_=false;
    unsigned interface_mask_=0;
    std::array<std::uint8_t,4> station_address_{};
    std::uint64_t onboarding_generation_=0;
    bool network_ready_=false,allow_migration_=false;
    runtime::Preparation* preparation_=nullptr;
    std::atomic<PreparationState> preparation_state_{PreparationState::not_requested};
    runtime::McuBootstrap* bootstrap_=nullptr;
    runtime::StartupDefaults startup_defaults_{};
    identity::Random startup_random_=nullptr;
    bool startup_configured_=false,startup_has_defaults_=false;
    std::atomic<runtime::StartupState> startup_state_{runtime::StartupState::not_requested};
};
}
