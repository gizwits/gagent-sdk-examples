// 由数据点生成器自动生成。请修改数据点定义后重新生成，不要手改本文件。
#pragma once
#include "product_config.hpp"
#include "gagent/protocol/protocol.hpp"
#include <array>
namespace gagent::board::datapoints {
inline constexpr char product_key[] = GAGENT_SOC_PRODUCT_KEY;
inline constexpr char business_protocol[] = "00000002";
// P0 操作码由平台 protocolType 决定，与 P0 版本号分别处理。
inline constexpr std::uint8_t write_action = 17;
inline constexpr std::uint8_t read_action = 18;
inline constexpr std::uint8_t read_response_action = 19;
inline constexpr std::uint8_t report_action = 20;
inline constexpr std::size_t count = 57;
inline constexpr std::array<const char*, count> field_names{{"start_stream", "feed_now", "feeding", "food_low", "cat_present", "enabled_2", "enabled_3", "enabled_4", "enabled_5", "enabled_6", "enabled_7", "enabled_8", "enabled_9", "enabled_10", "enabled_1", "feed_portion", "feed_result", "hour_1", "minute_1", "portion_1", "days_1", "hour_2", "minute_2", "portion_2", "days_2", "hour_3", "minute_3", "portion_3", "days_3", "hour_4", "minute_4", "portion_4", "days_4", "hour_5", "minute_5", "portion_5", "days_5", "hour_6", "minute_6", "portion_6", "days_6", "hour_7", "minute_7", "portion_7", "days_7", "hour_8", "minute_8", "portion_8", "days_8", "hour_9", "minute_9", "portion_9", "days_9", "hour_10", "minute_10", "portion_10", "days_10"}};
inline constexpr std::size_t max_payload = 53;
enum class Field : std::size_t {
    dp_start_stream = 0,
    dp_feed_now = 1,
    dp_feeding = 2,
    dp_food_low = 3,
    dp_cat_present = 4,
    dp_enabled_2 = 5,
    dp_enabled_3 = 6,
    dp_enabled_4 = 7,
    dp_enabled_5 = 8,
    dp_enabled_6 = 9,
    dp_enabled_7 = 10,
    dp_enabled_8 = 11,
    dp_enabled_9 = 12,
    dp_enabled_10 = 13,
    dp_enabled_1 = 14,
    dp_feed_portion = 15,
    dp_feed_result = 16,
    dp_hour_1 = 17,
    dp_minute_1 = 18,
    dp_portion_1 = 19,
    dp_days_1 = 20,
    dp_hour_2 = 21,
    dp_minute_2 = 22,
    dp_portion_2 = 23,
    dp_days_2 = 24,
    dp_hour_3 = 25,
    dp_minute_3 = 26,
    dp_portion_3 = 27,
    dp_days_3 = 28,
    dp_hour_4 = 29,
    dp_minute_4 = 30,
    dp_portion_4 = 31,
    dp_days_4 = 32,
    dp_hour_5 = 33,
    dp_minute_5 = 34,
    dp_portion_5 = 35,
    dp_days_5 = 36,
    dp_hour_6 = 37,
    dp_minute_6 = 38,
    dp_portion_6 = 39,
    dp_days_6 = 40,
    dp_hour_7 = 41,
    dp_minute_7 = 42,
    dp_portion_7 = 43,
    dp_days_7 = 44,
    dp_hour_8 = 45,
    dp_minute_8 = 46,
    dp_portion_8 = 47,
    dp_days_8 = 48,
    dp_hour_9 = 49,
    dp_minute_9 = 50,
    dp_portion_9 = 51,
    dp_days_9 = 52,
    dp_hour_10 = 53,
    dp_minute_10 = 54,
    dp_portion_10 = 55,
    dp_days_10 = 56,
};
using Fields = std::array<bool, count>;
struct DataPoints {
    bool value_start_stream = false;
    bool value_feed_now = false;
    bool value_feeding = false;
    bool value_food_low = false;
    bool value_cat_present = false;
    bool value_enabled_2 = false;
    bool value_enabled_3 = false;
    bool value_enabled_4 = false;
    bool value_enabled_5 = false;
    bool value_enabled_6 = false;
    bool value_enabled_7 = false;
    bool value_enabled_8 = false;
    bool value_enabled_9 = false;
    bool value_enabled_10 = false;
    bool value_enabled_1 = false;
    std::uint64_t value_feed_portion = 0;
    std::uint64_t value_feed_result = 0;
    std::uint64_t value_hour_1 = 0;
    std::uint64_t value_minute_1 = 0;
    std::uint64_t value_portion_1 = 0;
    std::uint64_t value_days_1 = 0;
    std::uint64_t value_hour_2 = 0;
    std::uint64_t value_minute_2 = 0;
    std::uint64_t value_portion_2 = 0;
    std::uint64_t value_days_2 = 0;
    std::uint64_t value_hour_3 = 0;
    std::uint64_t value_minute_3 = 0;
    std::uint64_t value_portion_3 = 0;
    std::uint64_t value_days_3 = 0;
    std::uint64_t value_hour_4 = 0;
    std::uint64_t value_minute_4 = 0;
    std::uint64_t value_portion_4 = 0;
    std::uint64_t value_days_4 = 0;
    std::uint64_t value_hour_5 = 0;
    std::uint64_t value_minute_5 = 0;
    std::uint64_t value_portion_5 = 0;
    std::uint64_t value_days_5 = 0;
    std::uint64_t value_hour_6 = 0;
    std::uint64_t value_minute_6 = 0;
    std::uint64_t value_portion_6 = 0;
    std::uint64_t value_days_6 = 0;
    std::uint64_t value_hour_7 = 0;
    std::uint64_t value_minute_7 = 0;
    std::uint64_t value_portion_7 = 0;
    std::uint64_t value_days_7 = 0;
    std::uint64_t value_hour_8 = 0;
    std::uint64_t value_minute_8 = 0;
    std::uint64_t value_portion_8 = 0;
    std::uint64_t value_days_8 = 0;
    std::uint64_t value_hour_9 = 0;
    std::uint64_t value_minute_9 = 0;
    std::uint64_t value_portion_9 = 0;
    std::uint64_t value_days_9 = 0;
    std::uint64_t value_hour_10 = 0;
    std::uint64_t value_minute_10 = 0;
    std::uint64_t value_portion_10 = 0;
    std::uint64_t value_days_10 = 0;
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
