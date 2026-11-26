#pragma once
#include "gagent/cloud/cloud.hpp"
namespace gagent::ota {
enum class Target {module=1,mcu=2,voice=7};
struct FirmwareVersion {std::string_view hardware,software;};
struct CheckMetadata {
    Target target=Target::mcu;
    std::string_view hardware,software,sdk_hardware,sdk_software;
    FirmwareVersion wifi,mcu,voice; // Missing voice fields use 00000000; absent MCU is omitted.
    std::string_view softap_name,station_name;
    bool report_build=false;
    std::string_view flash_size,build_time,redmine_id;
    std::uint32_t baud=0;
};
struct FirmwareReply {
    bool has_software=false,valid=true;
    std::uint32_t error_code=0;
    std::array<char,33> software{};std::array<char,640> url{};
};
struct CheckReply : FirmwareReply {
    bool multi_type=false;
    FirmwareReply wifi,mcu,voice;
};
using CheckJson=bool(*)(std::string_view,CheckReply&) noexcept;
struct Location {cloud::Address address;std::array<char,512> path{};};
enum class AutoCheckState {idle,waiting_module,running_module,waiting_mcu,running_mcu,complete,cancelled};
enum class BuildReportState {idle,pending,recorded,failed};
enum class CheckState {idle,queued,checking,no_update,available,failed,cancelled};
enum class CheckResult {invalid,no_update,available};
// Legacy lexical comparison (not semantic-version ordering). MD5/size arrive later in HTTP headers.
CheckResult check_result(std::string_view current,const FirmwareReply&,Location&) noexcept;
bool download_location(std::string_view,Location&) noexcept;
}
