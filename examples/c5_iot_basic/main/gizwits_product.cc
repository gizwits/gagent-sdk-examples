// 用户业务文件：configure_product.py 不会覆盖本文件。
// initialize() 设置初始值；apply() 执行下发；sample() 采集传感器。
#include "gizwits_product.hpp"
#include <algorithm>
namespace gagent::board::product {
namespace {
datapoints::DataPoints current{}, last_reported{};
bool first_report = true, pending = false;
std::uint64_t last_attempt = 0, last_full = 0;
}
void initialize() noexcept {
    current = {};
    // Set product defaults here (e.g. enabled LED, disabled schedules), using
    // the generated value_* members. JSON descriptions are not executable policy.
    last_reported = current;
    first_report = true;
    pending = false;
    last_attempt = last_full = 0;
}
bool apply(const datapoints::DataPoints& candidate, const datapoints::Fields& written) noexcept {
    (void)candidate;
    (void)written;
    // Implement relay/LED/timer writes here; Field::dp_* identifies each write.
    // Return false if the hardware rejects the transaction. No GPIOs are assumed.
    return true;
}
void sample(datapoints::DataPoints& candidate, std::uint64_t now) noexcept {
    (void)candidate;
    (void)now;
    // Read sensors/consume queued button events and update value_* here.
}
protocol::Status control(protocol::Bytes input, std::uint8_t* output,
                         std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (!input.data || !input.size) return protocol::Status::malformed;
    if (input.data[0] == datapoints::read_action) return datapoints::read_status(input, current, output, capacity, written);
    if (input.data[0] != datapoints::write_action) return protocol::Status::unsupported;
    auto candidate = current;
    datapoints::Fields fields{};
    const auto status = datapoints::decode_control({input.data+1, input.size-1}, candidate, fields);
    if (status != protocol::Status::ok) return status;
    if (!product::apply(candidate, fields)) return protocol::Status::handler_error;
    current = candidate;
    pending = true;
    // Legacy P0 control succeeds with an empty payload; the Router owns the ACK.
    return protocol::Status::ok;
}
void tick(reports::Service& reports, std::uint64_t now) noexcept {
    auto candidate = current;
    sample(candidate, now);
    if (datapoints::valid(candidate)) current = candidate;
    if (!first_report && now-last_attempt < 1000) return;
    auto fields = datapoints::changed(last_reported, current);
    const bool full = first_report || now-last_full >= 600000;
    if (full) fields = datapoints::all_fields();
    if (!full && !pending && std::none_of(fields.begin(), fields.end(), [](bool value) {return value;})) return;
    if (pending) fields = datapoints::all_fields();
    std::array<std::uint8_t, datapoints::max_payload> payload{};
    std::size_t size = 0;
    last_attempt = now;
    if (datapoints::encode_status(current, fields, datapoints::report_action, payload.data(), payload.size(), size) != protocol::Status::ok) return;
    if (reports.publish({payload.data(), size})) {
        // publish means fanout attempted, not acknowledged delivery; periodic full
        // reports and explicit status reads recover state for newly connected clients.
        last_reported = current;
        pending = first_report = false;
        if (full) last_full = now;
    }
}
}
