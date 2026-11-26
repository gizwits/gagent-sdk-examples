#pragma once
#include <array>
#include <cstdint>
namespace gagent::status {
struct State {
    bool ap=false,station=false,onboarding=false,binding=false;
    bool station_connected=false,cloud_connected=false,list_ready=false;
    std::uint8_t signal_level=0; // Legacy three-bit value, 0..7; signal_level() preserves the old RSSI mapping.
    bool client_online=false,factory_test=false,ble_enabled=false,ble_connected=false,ota=false;
};
// Legacy GAgent status word. Bit 7 stays reserved; the byte order is independent of the host.
bool encode(const State&,std::array<std::uint8_t,2>&) noexcept;
// Old gatNetGetSignalLevel thresholds; valid RSSI is supplied by the radio adapter.
std::uint8_t signal_level(std::int8_t rssi) noexcept;
class OtaActivity {
public:
    virtual ~OtaActivity()=default;
    virtual bool ota_active() const noexcept=0;
};
// Snapshot on the sole business worker. Return false until the source has reliable state.
class Source {
public:
    virtual ~Source()=default;
    virtual bool read(std::uint64_t now,State&) const noexcept=0;
    virtual bool ipv4(std::array<std::uint8_t,4>&) const noexcept {return false;}
};
}
