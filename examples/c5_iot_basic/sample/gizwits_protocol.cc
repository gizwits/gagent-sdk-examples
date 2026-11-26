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
    if (!input.data || input.size < 1) return Status::malformed;
    mask[0] = get_bits(input.data, 1, 0, 1) != 0;
    mask[1] = get_bits(input.data, 1, 1, 1) != 0;
    mask[2] = get_bits(input.data, 1, 2, 1) != 0;
    mask[3] = get_bits(input.data, 1, 3, 1) != 0;
    if (get_bits(input.data, 1, 4, 1)) return Status::malformed;
    if (get_bits(input.data, 1, 5, 1)) return Status::malformed;
    if (get_bits(input.data, 1, 6, 1)) return Status::malformed;
    if (get_bits(input.data, 1, 7, 1)) return Status::malformed;
    return Status::ok;
}
Status encode_values(const DataPoints& value, const Fields& mask,
                     std::uint8_t* output, std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (!valid(value)) return Status::malformed;
    (void)mask;
    constexpr std::size_t size = 5;
    if (!output || capacity < size) return Status::too_large;
    std::memset(output, 0, size);
    auto* values = output;
    [[maybe_unused]] std::uint32_t raw = 0;
    if (true) {
        put_bits(values + 0, 1, 0, 1, std::uint32_t(value.value_power));
    }
    if (true) {
        put_bits(values + 0, 1, 1, 1, std::uint32_t(value.value_using_led_light));
    }
    if (true) {
        put_bits(values + 0, 1, 2, 1, std::uint32_t(value.value_power_memory));
    }
    if (true) {
        if (!raw_value(double(value.value_timer_s), 1, 0, 0u, 86400u, raw)) return Status::malformed;
        put_uint(values + 1, 4, raw);
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
    mask[0] = before.value_power != after.value_power;
    mask[1] = before.value_using_led_light != after.value_using_led_light;
    mask[2] = before.value_power_memory != after.value_power_memory;
    mask[3] = before.value_timer_s != after.value_timer_s;
    return mask;
}
bool valid(const DataPoints& value) noexcept {
    (void)value;
    [[maybe_unused]] std::uint32_t raw = 0;
    if (!raw_value(double(value.value_timer_s), 1, 0, 0u, 86400u, raw)) return false;
    return true;
}
Status decode_control(protocol::Bytes input, DataPoints& state, Fields& written) noexcept {
    Fields mask{};
    const auto result = read_flags(input, mask);
    if (result != Status::ok) return result;
    if (input.size != 6) return Status::malformed;
    [[maybe_unused]] const auto* values = input.data + 1;
    auto next = state;
    if (mask[0]) {
        next.value_power = static_cast<bool>(get_bits(values + 0, 1, 0, 1));
    }
    if (mask[1]) {
        next.value_using_led_light = static_cast<bool>(get_bits(values + 0, 1, 1, 1));
    }
    if (mask[2]) {
        next.value_power_memory = static_cast<bool>(get_bits(values + 0, 1, 2, 1));
    }
    if (mask[3]) {
        const auto raw = get_uint(values + 1, 4);
        if (raw > 86400u) return Status::malformed;
        next.value_timer_s = static_cast<std::uint64_t>(double(raw) * 1 + 0);
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
    if (input.size != 1) return Status::malformed;
    const auto mask = all_fields();
    return encode_status(state, mask, read_response_action, output, capacity, written);
}
}
