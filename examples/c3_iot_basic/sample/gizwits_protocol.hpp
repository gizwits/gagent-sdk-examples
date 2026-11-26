// 由数据点生成器自动生成。请修改数据点定义后重新生成，不要手改本文件。
#pragma once
#include "product_config.hpp"
#include "gagent/protocol/protocol.hpp"
#include <array>
namespace gagent::board::datapoints {
inline constexpr char product_key[] = GAGENT_SOC_PRODUCT_KEY;
inline constexpr char business_protocol[] = "00000002";
// P0 操作码由平台 protocolType 决定，与 P0 版本号分别处理。
inline constexpr std::uint8_t write_action = 1;
inline constexpr std::uint8_t read_action = 2;
inline constexpr std::uint8_t read_response_action = 3;
inline constexpr std::uint8_t report_action = 4;
inline constexpr std::size_t count = 4;
inline constexpr std::array<const char*, count> field_names{{"power", "using_led_light", "power_memory", "timer_s"}};
inline constexpr std::size_t max_payload = 6;
enum class Field : std::size_t {
    dp_power = 0,
    dp_using_led_light = 1,
    dp_power_memory = 2,
    dp_timer_s = 3,
};
using Fields = std::array<bool, count>;
struct DataPoints {
    bool value_power = false;
    bool value_using_led_light = false;
    bool value_power_memory = false;
    std::uint64_t value_timer_s = 0;
};
Fields all_fields() noexcept;
Fields changed(const DataPoints& before, const DataPoints& after) noexcept;
bool valid(const DataPoints&) noexcept;
// 输入从字段标志 attr_flags 开始，调用前应去掉 P0 操作码。
// 输入无效时，不修改状态和已写字段集合。
protocol::Status decode_control(protocol::Bytes, DataPoints&, Fields&) noexcept;
// 使用查询响应或主动上报操作码，编码 P0 业务载荷。
protocol::Status encode_status(const DataPoints&, const Fields&, std::uint8_t action,
                              std::uint8_t*, std::size_t, std::size_t&) noexcept;
protocol::Status read_status(protocol::Bytes request, const DataPoints&,
                            std::uint8_t*, std::size_t, std::size_t&) noexcept;
}
