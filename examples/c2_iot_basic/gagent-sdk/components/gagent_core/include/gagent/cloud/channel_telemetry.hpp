#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
namespace gagent::c0 {
constexpr std::uint32_t default_interval=1800;
constexpr std::uint8_t channel=1,rssi=2,ip=4,interval=8,ssid=16,mac=32,all=63;
// Shared observation; no JSON, TLV, MQTT topics or cloud identities here.
struct Sample {
    std::uint8_t channel_type=1; // Wi-Fi, shared by every provider.
    std::array<std::uint8_t,6> mac{};
    std::array<std::uint8_t,4> ip{};
    std::array<std::uint8_t,32> ssid{};
    std::size_t ssid_size=0;
    std::int8_t rssi=0;
    std::uint32_t interval=default_interval;
};
struct Policy {
    bool enabled=false,changes=false,configurable=false;
    std::uint32_t interval_seconds=default_interval,first_delay_ms=0;
};
enum class Kind {report,query,set};
struct Request {
    Kind kind=Kind::report;
    std::uint8_t mask=all;
    std::uint32_t sequence=0,interval_seconds=0;
};
}
