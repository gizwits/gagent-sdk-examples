#pragma once
#include "gagent/cloud/mqtt.hpp"
#include "mqtt_client.h"
#include "gagent/mailbox.hpp"
#include "gagent/buffer_pool.hpp"
#include <atomic>
namespace gagent::platform {
// Runtime calls run on the worker and never wait for MQTT locks, DNS or sockets.
// Keep alive until idle(), after returning all leases; teardown runs on I/O.
class EspMqtt final : public mqtt::Driver {
public:
    explicit EspMqtt(mqtt::Payload::Reallocate reallocate=std::realloc) noexcept : reallocate_(reallocate) {}
    bool begin(const mqtt::Configuration&,std::uint32_t epoch) noexcept override;
    bool poll(mqtt::RxLease&) noexcept override;
    bool poll(mqtt::Event&) noexcept; // Copying convenience for external diagnostics/tests.
    bool reserve(mqtt::TxLease&,std::size_t capacity=512) noexcept override;
    bool submit(mqtt::TxLease&) noexcept override;
    bool publish(std::string_view,protocol::Bytes) noexcept override;
    bool publish(std::string_view,protocol::Bytes,unsigned qos) noexcept override;
    bool publish_reset(std::string_view,protocol::Bytes,unsigned qos) noexcept override;
    mqtt::ResetStatus reset_status() const noexcept override;
    void cancel() noexcept override {cancelled_.store(true);ready_.store(false);}
    bool busy() const noexcept override {return busy_.load();}
    bool idle() const noexcept override {return !busy_.load() && !borrowed_ && !reserved_;}
    // Approximate concurrent snapshots: used includes assembling/borrowed slots;
    // byte capacities remain allocated even when the corresponding slot is idle.
    struct BufferUsage {unsigned rx_bytes=0,tx_bytes=0,rx_used=0,tx_used=0,rx_peak=0,tx_peak=0,rx_waits=0,rx_overflows=0,tx_full=0,rx_oom=0,tx_oom=0;};
    BufferUsage buffer_usage() const noexcept;
    mqtt::ConnectionState connection_state() const noexcept override {
        if(cancelled_.load() || failure_.load() || !busy_.load())return mqtt::ConnectionState::failed;
        return ready_.load()?mqtt::ConnectionState::ready:mqtt::ConnectionState::pending;
    }
private:
    struct EventIndex {mqtt::EventKind kind=mqtt::EventKind::disconnected;std::uint32_t epoch=0;mqtt::BufferToken token{};};
    static void release_rx(void*,mqtt::BufferToken) noexcept;
    static void release_tx(void*,mqtt::BufferToken) noexcept;
    void discard_events() noexcept; // Worker, after producer has stopped.
    void discard_incoming() noexcept; // SDK callback, or I/O after SDK stop.
    void transmit(mqtt::BufferToken,bool direct) noexcept;
    bool queue(std::string_view,protocol::Bytes,unsigned qos,bool reset) noexcept;
    void reset_published(const mqtt::Publish&,int result) noexcept;
    void drain_acks() noexcept;
    static void entry(void*) noexcept;
    static void event(void*,esp_event_base_t,std::int32_t,void*) noexcept;
    void receive(esp_mqtt_event_handle_t) noexcept;
    void run() noexcept;
    void log_stack(bool force=false) noexcept;
    void log_buffers(bool force=false) noexcept;
    void log_state(const char* phase) const noexcept;
    mqtt::BufferToken acquire_rx(std::size_t capacity) noexcept;
    // Each task samples its own live stack; never query an SDK handle after teardown.
    struct StackUsage {std::uint32_t tick=0;unsigned reported=UINT32_MAX;};
    void* io_task_=nullptr;
    StackUsage io_stack_{},sdk_stack_{};
    mqtt::Configuration configuration_{};
    std::uint32_t epoch_=0;
#ifdef GAGENT_CONNECTION_TIMING
    std::int64_t connect_started_us_=0,subscribe_started_us_=0;
#endif
    esp_mqtt_client_handle_t client_=nullptr;
    int subscription_=-1,fragment_id_=0;
    mqtt::Assembler assembler_;
    BufferPool<mqtt::Event,3> rx_; // Assembling + queued + worker-owned.
    BufferPool<mqtt::Publish,2> tx_; // I/O-owned + queued/reserved.
    mqtt::Payload::Reallocate reallocate_;
    std::array<std::atomic<unsigned>,3> rx_capacity_{};
    std::array<std::atomic<unsigned>,2> tx_capacity_{};
    std::atomic<unsigned> rx_peak_{0},tx_peak_{0},rx_waits_{0},rx_overflows_{0},tx_full_{0},rx_oom_{0},tx_oom_{0};
    std::uint32_t buffers_tick_=0; // I/O task only; diagnostics are cumulative across reconnects.
    mqtt::BufferToken incoming_{}; // SDK callback; I/O only after SDK stop.
    mqtt::BufferToken borrowed_{},reserved_{}; // Worker only; never forcibly reclaimed.
    Mailbox<EventIndex,4> events_; // Can queue all RX slots when worker/assembler are idle.
    Mailbox<mqtt::BufferToken,3> publishes_; // Two small indices allow a back-to-back ACK + report.
    Mailbox<int,4> reset_acks_; // Callback -> I/O task; handles a PUBACK arriving before publish returns.
    int reset_id_=-1; // I/O task only.
    std::atomic<mqtt::ResetStatus> reset_status_{mqtt::ResetStatus::idle};
    std::atomic<bool> busy_{false},cancelled_{false},ready_{false};
    std::atomic<unsigned> submitting_{0}; // Close admission before draining the producer.
    std::atomic<int> failure_{0}; // 1 transport/overflow, 2 authentication.
};
}
