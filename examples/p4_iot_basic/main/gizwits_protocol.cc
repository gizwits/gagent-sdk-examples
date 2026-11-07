// 由数据点生成器自动生成；不动态分配内存，不依赖结构体布局或主机字节序。
#include "gizwits_protocol.hpp"
#include <cmath>
#include <cstring>
namespace gagent::board::datapoints {
using protocol::Status;
namespace {
// 字段标志和独立位段按大端字节数组保存，位编号从整段的最低位开始。
[[maybe_unused]] std::uint32_t get_bits(const std::uint8_t* data, std::size_t bytes,
                       std::size_t start, unsigned width) noexcept {
    std::uint32_t value = 0;
    for (unsigned i=0; i<width; ++i) {
        const auto bit = start+i;
        value |= std::uint32_t((data[bytes-1-bit/8] >> (bit%8)) & 1u) << i;
    }
    return value;
}
[[maybe_unused]] void put_bits(std::uint8_t* data, std::size_t bytes, std::size_t start,
              unsigned width, std::uint32_t value) noexcept {
    for (unsigned i=0; i<width; ++i) {
        const auto bit = start+i;
        data[bytes-1-bit/8] |= std::uint8_t(((value>>i)&1u) << (bit%8));
    }
}
[[maybe_unused]] std::uint32_t get_uint(const std::uint8_t* data, unsigned bytes) noexcept {
    std::uint32_t value = 0;
    for (unsigned i=0; i<bytes; ++i) value = (value<<8) | data[i];
    return value;
}
[[maybe_unused]] void put_uint(std::uint8_t* data, unsigned bytes, std::uint32_t value) noexcept {
    for (unsigned i=0; i<bytes; ++i) data[bytes-1-i] = std::uint8_t(value>>(8*i));
}
// 允许浮点表示误差，但拒绝不符合数据点步长的数值，避免静默截断。
[[maybe_unused]] bool raw_value(double value, double ratio, double addition, std::uint32_t minimum,
               std::uint32_t maximum, std::uint32_t& output) noexcept {
    if (!std::isfinite(value)) return false;
    const double raw = (value-addition)/ratio;
    if (!std::isfinite(raw)) return false;
    const double rounded = std::round(raw);
    if (rounded < minimum || rounded > maximum || std::abs(raw-rounded) > 0.00001) return false;
    output = static_cast<std::uint32_t>(rounded);
    return true;
}
Status read_flags(protocol::Bytes input, Fields& mask) noexcept {
    (void)mask;
    if (!input.data || input.size < 8) return Status::malformed;
    mask[0] = get_bits(input.data, 8, 0, 1) != 0;
    mask[1] = get_bits(input.data, 8, 1, 1) != 0;
    mask[2] = get_bits(input.data, 8, 2, 1) != 0;
    mask[3] = get_bits(input.data, 8, 3, 1) != 0;
    mask[4] = get_bits(input.data, 8, 4, 1) != 0;
    mask[5] = get_bits(input.data, 8, 5, 1) != 0;
    mask[6] = get_bits(input.data, 8, 6, 1) != 0;
    mask[7] = get_bits(input.data, 8, 7, 1) != 0;
    mask[8] = get_bits(input.data, 8, 8, 1) != 0;
    mask[9] = get_bits(input.data, 8, 9, 1) != 0;
    mask[10] = get_bits(input.data, 8, 10, 1) != 0;
    mask[11] = get_bits(input.data, 8, 11, 1) != 0;
    mask[12] = get_bits(input.data, 8, 12, 1) != 0;
    mask[13] = get_bits(input.data, 8, 13, 1) != 0;
    mask[14] = get_bits(input.data, 8, 14, 1) != 0;
    mask[15] = get_bits(input.data, 8, 15, 1) != 0;
    mask[16] = get_bits(input.data, 8, 16, 1) != 0;
    mask[17] = get_bits(input.data, 8, 17, 1) != 0;
    mask[18] = get_bits(input.data, 8, 18, 1) != 0;
    mask[19] = get_bits(input.data, 8, 19, 1) != 0;
    mask[20] = get_bits(input.data, 8, 20, 1) != 0;
    mask[21] = get_bits(input.data, 8, 21, 1) != 0;
    mask[22] = get_bits(input.data, 8, 22, 1) != 0;
    mask[23] = get_bits(input.data, 8, 23, 1) != 0;
    mask[24] = get_bits(input.data, 8, 24, 1) != 0;
    mask[25] = get_bits(input.data, 8, 25, 1) != 0;
    mask[26] = get_bits(input.data, 8, 26, 1) != 0;
    mask[27] = get_bits(input.data, 8, 27, 1) != 0;
    mask[28] = get_bits(input.data, 8, 28, 1) != 0;
    mask[29] = get_bits(input.data, 8, 29, 1) != 0;
    mask[30] = get_bits(input.data, 8, 30, 1) != 0;
    mask[31] = get_bits(input.data, 8, 31, 1) != 0;
    mask[32] = get_bits(input.data, 8, 32, 1) != 0;
    mask[33] = get_bits(input.data, 8, 33, 1) != 0;
    mask[34] = get_bits(input.data, 8, 34, 1) != 0;
    mask[35] = get_bits(input.data, 8, 35, 1) != 0;
    mask[36] = get_bits(input.data, 8, 36, 1) != 0;
    mask[37] = get_bits(input.data, 8, 37, 1) != 0;
    mask[38] = get_bits(input.data, 8, 38, 1) != 0;
    mask[39] = get_bits(input.data, 8, 39, 1) != 0;
    mask[40] = get_bits(input.data, 8, 40, 1) != 0;
    mask[41] = get_bits(input.data, 8, 41, 1) != 0;
    mask[42] = get_bits(input.data, 8, 42, 1) != 0;
    mask[43] = get_bits(input.data, 8, 43, 1) != 0;
    mask[44] = get_bits(input.data, 8, 44, 1) != 0;
    mask[45] = get_bits(input.data, 8, 45, 1) != 0;
    mask[46] = get_bits(input.data, 8, 46, 1) != 0;
    mask[47] = get_bits(input.data, 8, 47, 1) != 0;
    mask[48] = get_bits(input.data, 8, 48, 1) != 0;
    mask[49] = get_bits(input.data, 8, 49, 1) != 0;
    mask[50] = get_bits(input.data, 8, 50, 1) != 0;
    mask[51] = get_bits(input.data, 8, 51, 1) != 0;
    mask[52] = get_bits(input.data, 8, 52, 1) != 0;
    mask[53] = get_bits(input.data, 8, 53, 1) != 0;
    mask[54] = get_bits(input.data, 8, 54, 1) != 0;
    mask[55] = get_bits(input.data, 8, 55, 1) != 0;
    mask[56] = get_bits(input.data, 8, 56, 1) != 0;
    if (get_bits(input.data, 8, 57, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 58, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 59, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 60, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 61, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 62, 1)) return Status::malformed;
    if (get_bits(input.data, 8, 63, 1)) return Status::malformed;
    return Status::ok;
}
Status encode_values(const DataPoints& value, const Fields& mask,
                     std::uint8_t* output, std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (!valid(value)) return Status::malformed;
    std::size_t bit_count = 0, byte_count = 0;
    if (mask[0]) bit_count += 1;
    if (mask[1]) bit_count += 1;
    if (mask[2]) bit_count += 1;
    if (mask[3]) bit_count += 1;
    if (mask[4]) bit_count += 1;
    if (mask[5]) bit_count += 1;
    if (mask[6]) bit_count += 1;
    if (mask[7]) bit_count += 1;
    if (mask[8]) bit_count += 1;
    if (mask[9]) bit_count += 1;
    if (mask[10]) bit_count += 1;
    if (mask[11]) bit_count += 1;
    if (mask[12]) bit_count += 1;
    if (mask[13]) bit_count += 1;
    if (mask[14]) bit_count += 1;
    if (mask[15]) byte_count += 1;
    if (mask[16]) byte_count += 1;
    if (mask[17]) byte_count += 1;
    if (mask[18]) byte_count += 1;
    if (mask[19]) byte_count += 1;
    if (mask[20]) byte_count += 1;
    if (mask[21]) byte_count += 1;
    if (mask[22]) byte_count += 1;
    if (mask[23]) byte_count += 1;
    if (mask[24]) byte_count += 1;
    if (mask[25]) byte_count += 1;
    if (mask[26]) byte_count += 1;
    if (mask[27]) byte_count += 1;
    if (mask[28]) byte_count += 1;
    if (mask[29]) byte_count += 1;
    if (mask[30]) byte_count += 1;
    if (mask[31]) byte_count += 1;
    if (mask[32]) byte_count += 1;
    if (mask[33]) byte_count += 1;
    if (mask[34]) byte_count += 1;
    if (mask[35]) byte_count += 1;
    if (mask[36]) byte_count += 1;
    if (mask[37]) byte_count += 1;
    if (mask[38]) byte_count += 1;
    if (mask[39]) byte_count += 1;
    if (mask[40]) byte_count += 1;
    if (mask[41]) byte_count += 1;
    if (mask[42]) byte_count += 1;
    if (mask[43]) byte_count += 1;
    if (mask[44]) byte_count += 1;
    if (mask[45]) byte_count += 1;
    if (mask[46]) byte_count += 1;
    if (mask[47]) byte_count += 1;
    if (mask[48]) byte_count += 1;
    if (mask[49]) byte_count += 1;
    if (mask[50]) byte_count += 1;
    if (mask[51]) byte_count += 1;
    if (mask[52]) byte_count += 1;
    if (mask[53]) byte_count += 1;
    if (mask[54]) byte_count += 1;
    if (mask[55]) byte_count += 1;
    if (mask[56]) byte_count += 1;
    const auto bit_bytes = (bit_count+7)/8;
    const auto size = 8+bit_bytes+byte_count;
    if (!output || capacity < size) return Status::too_large;
    std::memset(output, 0, size);
    put_bits(output, 8, 0, 1, mask[0]);
    put_bits(output, 8, 1, 1, mask[1]);
    put_bits(output, 8, 2, 1, mask[2]);
    put_bits(output, 8, 3, 1, mask[3]);
    put_bits(output, 8, 4, 1, mask[4]);
    put_bits(output, 8, 5, 1, mask[5]);
    put_bits(output, 8, 6, 1, mask[6]);
    put_bits(output, 8, 7, 1, mask[7]);
    put_bits(output, 8, 8, 1, mask[8]);
    put_bits(output, 8, 9, 1, mask[9]);
    put_bits(output, 8, 10, 1, mask[10]);
    put_bits(output, 8, 11, 1, mask[11]);
    put_bits(output, 8, 12, 1, mask[12]);
    put_bits(output, 8, 13, 1, mask[13]);
    put_bits(output, 8, 14, 1, mask[14]);
    put_bits(output, 8, 15, 1, mask[15]);
    put_bits(output, 8, 16, 1, mask[16]);
    put_bits(output, 8, 17, 1, mask[17]);
    put_bits(output, 8, 18, 1, mask[18]);
    put_bits(output, 8, 19, 1, mask[19]);
    put_bits(output, 8, 20, 1, mask[20]);
    put_bits(output, 8, 21, 1, mask[21]);
    put_bits(output, 8, 22, 1, mask[22]);
    put_bits(output, 8, 23, 1, mask[23]);
    put_bits(output, 8, 24, 1, mask[24]);
    put_bits(output, 8, 25, 1, mask[25]);
    put_bits(output, 8, 26, 1, mask[26]);
    put_bits(output, 8, 27, 1, mask[27]);
    put_bits(output, 8, 28, 1, mask[28]);
    put_bits(output, 8, 29, 1, mask[29]);
    put_bits(output, 8, 30, 1, mask[30]);
    put_bits(output, 8, 31, 1, mask[31]);
    put_bits(output, 8, 32, 1, mask[32]);
    put_bits(output, 8, 33, 1, mask[33]);
    put_bits(output, 8, 34, 1, mask[34]);
    put_bits(output, 8, 35, 1, mask[35]);
    put_bits(output, 8, 36, 1, mask[36]);
    put_bits(output, 8, 37, 1, mask[37]);
    put_bits(output, 8, 38, 1, mask[38]);
    put_bits(output, 8, 39, 1, mask[39]);
    put_bits(output, 8, 40, 1, mask[40]);
    put_bits(output, 8, 41, 1, mask[41]);
    put_bits(output, 8, 42, 1, mask[42]);
    put_bits(output, 8, 43, 1, mask[43]);
    put_bits(output, 8, 44, 1, mask[44]);
    put_bits(output, 8, 45, 1, mask[45]);
    put_bits(output, 8, 46, 1, mask[46]);
    put_bits(output, 8, 47, 1, mask[47]);
    put_bits(output, 8, 48, 1, mask[48]);
    put_bits(output, 8, 49, 1, mask[49]);
    put_bits(output, 8, 50, 1, mask[50]);
    put_bits(output, 8, 51, 1, mask[51]);
    put_bits(output, 8, 52, 1, mask[52]);
    put_bits(output, 8, 53, 1, mask[53]);
    put_bits(output, 8, 54, 1, mask[54]);
    put_bits(output, 8, 55, 1, mask[55]);
    put_bits(output, 8, 56, 1, mask[56]);
    auto* values = output + 8;
    std::size_t bit_offset = 0, byte_offset = bit_bytes;
    (void)bit_offset; (void)byte_offset;
    [[maybe_unused]] std::uint32_t raw = 0;
    if (mask[0]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_start_stream)); bit_offset += 1;
    }
    if (mask[1]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_feed_now)); bit_offset += 1;
    }
    if (mask[2]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_feeding)); bit_offset += 1;
    }
    if (mask[3]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_food_low)); bit_offset += 1;
    }
    if (mask[4]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_cat_present)); bit_offset += 1;
    }
    if (mask[5]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_2)); bit_offset += 1;
    }
    if (mask[6]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_3)); bit_offset += 1;
    }
    if (mask[7]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_4)); bit_offset += 1;
    }
    if (mask[8]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_5)); bit_offset += 1;
    }
    if (mask[9]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_6)); bit_offset += 1;
    }
    if (mask[10]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_7)); bit_offset += 1;
    }
    if (mask[11]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_8)); bit_offset += 1;
    }
    if (mask[12]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_9)); bit_offset += 1;
    }
    if (mask[13]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_10)); bit_offset += 1;
    }
    if (mask[14]) {
        put_bits(values, bit_bytes, bit_offset, 1, std::uint32_t(value.value_enabled_1)); bit_offset += 1;
    }
    if (mask[15]) {
        if (!raw_value(double(value.value_feed_portion), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[16]) {
        if (!raw_value(double(value.value_feed_result), 1, 0, 0u, 10u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[17]) {
        if (!raw_value(double(value.value_hour_1), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[18]) {
        if (!raw_value(double(value.value_minute_1), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[19]) {
        if (!raw_value(double(value.value_portion_1), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[20]) {
        if (!raw_value(double(value.value_days_1), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[21]) {
        if (!raw_value(double(value.value_hour_2), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[22]) {
        if (!raw_value(double(value.value_minute_2), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[23]) {
        if (!raw_value(double(value.value_portion_2), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[24]) {
        if (!raw_value(double(value.value_days_2), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[25]) {
        if (!raw_value(double(value.value_hour_3), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[26]) {
        if (!raw_value(double(value.value_minute_3), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[27]) {
        if (!raw_value(double(value.value_portion_3), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[28]) {
        if (!raw_value(double(value.value_days_3), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[29]) {
        if (!raw_value(double(value.value_hour_4), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[30]) {
        if (!raw_value(double(value.value_minute_4), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[31]) {
        if (!raw_value(double(value.value_portion_4), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[32]) {
        if (!raw_value(double(value.value_days_4), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[33]) {
        if (!raw_value(double(value.value_hour_5), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[34]) {
        if (!raw_value(double(value.value_minute_5), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[35]) {
        if (!raw_value(double(value.value_portion_5), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[36]) {
        if (!raw_value(double(value.value_days_5), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[37]) {
        if (!raw_value(double(value.value_hour_6), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[38]) {
        if (!raw_value(double(value.value_minute_6), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[39]) {
        if (!raw_value(double(value.value_portion_6), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[40]) {
        if (!raw_value(double(value.value_days_6), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[41]) {
        if (!raw_value(double(value.value_hour_7), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[42]) {
        if (!raw_value(double(value.value_minute_7), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[43]) {
        if (!raw_value(double(value.value_portion_7), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[44]) {
        if (!raw_value(double(value.value_days_7), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[45]) {
        if (!raw_value(double(value.value_hour_8), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[46]) {
        if (!raw_value(double(value.value_minute_8), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[47]) {
        if (!raw_value(double(value.value_portion_8), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[48]) {
        if (!raw_value(double(value.value_days_8), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[49]) {
        if (!raw_value(double(value.value_hour_9), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[50]) {
        if (!raw_value(double(value.value_minute_9), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[51]) {
        if (!raw_value(double(value.value_portion_9), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[52]) {
        if (!raw_value(double(value.value_days_9), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[53]) {
        if (!raw_value(double(value.value_hour_10), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[54]) {
        if (!raw_value(double(value.value_minute_10), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[55]) {
        if (!raw_value(double(value.value_portion_10), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    if (mask[56]) {
        if (!raw_value(double(value.value_days_10), 1, 0, 0u, 255u, raw)) return Status::malformed;
        put_uint(values + byte_offset, 1, raw); byte_offset += 1;
    }
    written = size;
    return Status::ok;
}
}
Fields all_fields() noexcept {
    Fields result{};
    result.fill(true);
    return result;
}
Fields changed(const DataPoints& before, const DataPoints& after) noexcept {
    Fields mask{};
    mask[0] = before.value_start_stream != after.value_start_stream;
    mask[1] = before.value_feed_now != after.value_feed_now;
    mask[2] = before.value_feeding != after.value_feeding;
    mask[3] = before.value_food_low != after.value_food_low;
    mask[4] = before.value_cat_present != after.value_cat_present;
    mask[5] = before.value_enabled_2 != after.value_enabled_2;
    mask[6] = before.value_enabled_3 != after.value_enabled_3;
    mask[7] = before.value_enabled_4 != after.value_enabled_4;
    mask[8] = before.value_enabled_5 != after.value_enabled_5;
    mask[9] = before.value_enabled_6 != after.value_enabled_6;
    mask[10] = before.value_enabled_7 != after.value_enabled_7;
    mask[11] = before.value_enabled_8 != after.value_enabled_8;
    mask[12] = before.value_enabled_9 != after.value_enabled_9;
    mask[13] = before.value_enabled_10 != after.value_enabled_10;
    mask[14] = before.value_enabled_1 != after.value_enabled_1;
    mask[15] = before.value_feed_portion != after.value_feed_portion;
    mask[16] = before.value_feed_result != after.value_feed_result;
    mask[17] = before.value_hour_1 != after.value_hour_1;
    mask[18] = before.value_minute_1 != after.value_minute_1;
    mask[19] = before.value_portion_1 != after.value_portion_1;
    mask[20] = before.value_days_1 != after.value_days_1;
    mask[21] = before.value_hour_2 != after.value_hour_2;
    mask[22] = before.value_minute_2 != after.value_minute_2;
    mask[23] = before.value_portion_2 != after.value_portion_2;
    mask[24] = before.value_days_2 != after.value_days_2;
    mask[25] = before.value_hour_3 != after.value_hour_3;
    mask[26] = before.value_minute_3 != after.value_minute_3;
    mask[27] = before.value_portion_3 != after.value_portion_3;
    mask[28] = before.value_days_3 != after.value_days_3;
    mask[29] = before.value_hour_4 != after.value_hour_4;
    mask[30] = before.value_minute_4 != after.value_minute_4;
    mask[31] = before.value_portion_4 != after.value_portion_4;
    mask[32] = before.value_days_4 != after.value_days_4;
    mask[33] = before.value_hour_5 != after.value_hour_5;
    mask[34] = before.value_minute_5 != after.value_minute_5;
    mask[35] = before.value_portion_5 != after.value_portion_5;
    mask[36] = before.value_days_5 != after.value_days_5;
    mask[37] = before.value_hour_6 != after.value_hour_6;
    mask[38] = before.value_minute_6 != after.value_minute_6;
    mask[39] = before.value_portion_6 != after.value_portion_6;
    mask[40] = before.value_days_6 != after.value_days_6;
    mask[41] = before.value_hour_7 != after.value_hour_7;
    mask[42] = before.value_minute_7 != after.value_minute_7;
    mask[43] = before.value_portion_7 != after.value_portion_7;
    mask[44] = before.value_days_7 != after.value_days_7;
    mask[45] = before.value_hour_8 != after.value_hour_8;
    mask[46] = before.value_minute_8 != after.value_minute_8;
    mask[47] = before.value_portion_8 != after.value_portion_8;
    mask[48] = before.value_days_8 != after.value_days_8;
    mask[49] = before.value_hour_9 != after.value_hour_9;
    mask[50] = before.value_minute_9 != after.value_minute_9;
    mask[51] = before.value_portion_9 != after.value_portion_9;
    mask[52] = before.value_days_9 != after.value_days_9;
    mask[53] = before.value_hour_10 != after.value_hour_10;
    mask[54] = before.value_minute_10 != after.value_minute_10;
    mask[55] = before.value_portion_10 != after.value_portion_10;
    mask[56] = before.value_days_10 != after.value_days_10;
    return mask;
}
bool valid(const DataPoints& value) noexcept {
    (void)value;
    [[maybe_unused]] std::uint32_t raw = 0;
    if (!raw_value(double(value.value_feed_portion), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_feed_result), 1, 0, 0u, 10u, raw)) return false;
    if (!raw_value(double(value.value_hour_1), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_1), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_1), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_1), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_2), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_2), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_2), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_2), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_3), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_3), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_3), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_3), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_4), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_4), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_4), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_4), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_5), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_5), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_5), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_5), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_6), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_6), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_6), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_6), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_7), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_7), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_7), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_7), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_8), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_8), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_8), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_8), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_9), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_9), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_9), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_9), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_hour_10), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_minute_10), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_portion_10), 1, 0, 0u, 255u, raw)) return false;
    if (!raw_value(double(value.value_days_10), 1, 0, 0u, 255u, raw)) return false;
    return true;
}
Status decode_control(protocol::Bytes input, DataPoints& state, Fields& written) noexcept {
    Fields mask{};
    const auto result = read_flags(input, mask);
    if (result != Status::ok) return result;
    std::size_t bit_count = 0, byte_count = 0;
    if (mask[0]) bit_count += 1;
    if (mask[1]) bit_count += 1;
    if (mask[2]) bit_count += 1;
    if (mask[3]) bit_count += 1;
    if (mask[4]) bit_count += 1;
    if (mask[5]) bit_count += 1;
    if (mask[6]) bit_count += 1;
    if (mask[7]) bit_count += 1;
    if (mask[8]) bit_count += 1;
    if (mask[9]) bit_count += 1;
    if (mask[10]) bit_count += 1;
    if (mask[11]) bit_count += 1;
    if (mask[12]) bit_count += 1;
    if (mask[13]) bit_count += 1;
    if (mask[14]) bit_count += 1;
    if (mask[15]) byte_count += 1;
    if (mask[16]) byte_count += 1;
    if (mask[17]) byte_count += 1;
    if (mask[18]) byte_count += 1;
    if (mask[19]) byte_count += 1;
    if (mask[20]) byte_count += 1;
    if (mask[21]) byte_count += 1;
    if (mask[22]) byte_count += 1;
    if (mask[23]) byte_count += 1;
    if (mask[24]) byte_count += 1;
    if (mask[25]) byte_count += 1;
    if (mask[26]) byte_count += 1;
    if (mask[27]) byte_count += 1;
    if (mask[28]) byte_count += 1;
    if (mask[29]) byte_count += 1;
    if (mask[30]) byte_count += 1;
    if (mask[31]) byte_count += 1;
    if (mask[32]) byte_count += 1;
    if (mask[33]) byte_count += 1;
    if (mask[34]) byte_count += 1;
    if (mask[35]) byte_count += 1;
    if (mask[36]) byte_count += 1;
    if (mask[37]) byte_count += 1;
    if (mask[38]) byte_count += 1;
    if (mask[39]) byte_count += 1;
    if (mask[40]) byte_count += 1;
    if (mask[41]) byte_count += 1;
    if (mask[42]) byte_count += 1;
    if (mask[43]) byte_count += 1;
    if (mask[44]) byte_count += 1;
    if (mask[45]) byte_count += 1;
    if (mask[46]) byte_count += 1;
    if (mask[47]) byte_count += 1;
    if (mask[48]) byte_count += 1;
    if (mask[49]) byte_count += 1;
    if (mask[50]) byte_count += 1;
    if (mask[51]) byte_count += 1;
    if (mask[52]) byte_count += 1;
    if (mask[53]) byte_count += 1;
    if (mask[54]) byte_count += 1;
    if (mask[55]) byte_count += 1;
    if (mask[56]) byte_count += 1;
    const auto bit_bytes = (bit_count+7)/8;
    if (input.size != 8+bit_bytes+byte_count) return Status::malformed;
    [[maybe_unused]] const auto* values = input.data + 8;
    std::size_t bit_offset = 0, byte_offset = bit_bytes;
    (void)bit_offset; (void)byte_offset;
    auto next = state;
    if (mask[0]) {
        next.value_start_stream = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[1]) {
        next.value_feed_now = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[2]) return Status::malformed;
    if (mask[3]) return Status::malformed;
    if (mask[4]) return Status::malformed;
    if (mask[5]) {
        next.value_enabled_2 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[6]) {
        next.value_enabled_3 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[7]) {
        next.value_enabled_4 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[8]) {
        next.value_enabled_5 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[9]) {
        next.value_enabled_6 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[10]) {
        next.value_enabled_7 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[11]) {
        next.value_enabled_8 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[12]) {
        next.value_enabled_9 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[13]) {
        next.value_enabled_10 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[14]) {
        next.value_enabled_1 = static_cast<bool>(get_bits(values, bit_bytes, bit_offset, 1));
        bit_offset += 1;
    }
    if (mask[15]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_feed_portion = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[16]) return Status::malformed;
    if (mask[17]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_1 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[18]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_1 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[19]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_1 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[20]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_1 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[21]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_2 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[22]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_2 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[23]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_2 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[24]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_2 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[25]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_3 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[26]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_3 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[27]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_3 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[28]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_3 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[29]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_4 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[30]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_4 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[31]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_4 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[32]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_4 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[33]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_5 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[34]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_5 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[35]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_5 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[36]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_5 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[37]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_6 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[38]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_6 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[39]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_6 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[40]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_6 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[41]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_7 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[42]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_7 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[43]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_7 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[44]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_7 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[45]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_8 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[46]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_8 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[47]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_8 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[48]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_8 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[49]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_9 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[50]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_9 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[51]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_9 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[52]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_9 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[53]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_hour_10 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[54]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_minute_10 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[55]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_portion_10 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (mask[56]) {
        const auto raw = get_uint(values + byte_offset, 1);
        if (raw > 255u) return Status::malformed;
        next.value_days_10 = static_cast<std::uint64_t>(double(raw) * 1 + 0);
        byte_offset += 1;
    }
    if (!valid(next)) return Status::malformed;
    state = next;
    written = mask;
    return Status::ok;
}
Status encode_status(const DataPoints& value, const Fields& mask, std::uint8_t action,
                     std::uint8_t* output, std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (action != read_response_action && action != report_action) return Status::unsupported;
    if (!output || capacity < 1) return Status::too_large;
    std::size_t size = 0;
    const auto result = encode_values(value, mask, output+1, capacity-1, size);
    if (result != Status::ok) return result;
    output[0] = action;
    written = size+1;
    return Status::ok;
}
Status read_status(protocol::Bytes input, const DataPoints& state,
                   std::uint8_t* output, std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (!input.data || !input.size || input.data[0] != read_action) return Status::malformed;
    if (input.size != 1+8) return Status::malformed;
    // 全量查询将整个字段标志数组填为 FF，包括末尾填充位。
    bool full = true;
    for (std::size_t i=1; i<input.size; ++i) full = full && input.data[i] == 0xff;
    Fields mask{};
    if (full) mask = all_fields();
    else {
        const auto status = read_flags({input.data+1, input.size-1}, mask);
        if (status != Status::ok) return status;
    }
    return encode_status(state, mask, read_response_action, output, capacity, written);
}
}
