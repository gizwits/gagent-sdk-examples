#pragma once
#include "gagent/ota/ota_check.hpp"
#include "gagent/application/runtime.hpp"
namespace gagent::ota {
constexpr unsigned version_attempt_limit=30;
enum class Admission {allowed,allowed_volatile,exhausted,invalid};
// One runtime owner; Wi-Fi and MCU each retain their latest candidate. Persist
// best-effort before download and keep a RAM budget when storage is unavailable.
// Identity excludes URLs, current software and device credentials.
class AttemptBudget {
public:
    Admission admit(runtime::Boot&,storage::Md5,Target,std::string_view provider,
                    std::string_view product,std::string_view hardware,std::string_view version,
                    bool hardware_identity=true,std::uint64_t unix_seconds=0,std::uint64_t now_ms=0) noexcept;
private:
    struct Entry {std::array<std::uint8_t,16> identity{};unsigned attempts=0;std::uint64_t day=0,window_start=0;bool timed=false;};
    std::array<Entry,2> entries_{};
};
}
