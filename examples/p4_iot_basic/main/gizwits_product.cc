// 客户业务文件：更新数据点协议时，生成器不会覆盖此文件。
#include "gizwits_product.hpp"
#include <algorithm>
#include <atomic>
#include "gagent/application/application.hpp"
#include "esp_log.h"
#include "hardware.hpp"
#include "memory_log.hpp"
#ifdef GAGENT_CAMERA_ENABLED
#include "gagent/camera/service.hpp"
#endif
namespace gagent::board::product {
namespace {
runtime::Application* application = nullptr;
runtime::Boot* boot = nullptr;
// 跨任务只传递重新配网意图，不跨任务调用 Application。
std::atomic<bool> pairing_requested{false};
bool online = false;
ui::Button button;
bool hardware_attempted=false, hardware_ready=false;
// 先等待 SDK 接受请求，再修改数据点；排队成功不等于已连接推流。
bool request_stream(bool start) noexcept {
#ifdef GAGENT_CAMERA_ENABLED
    const auto result=camera::service().request(start ? 1 : 2);
    ESP_LOGI("sdk_basic", "Stream request=%s result=%u", start ? "start" : "stop", unsigned(result));
    return result==0;
#else
    if (start) ESP_LOGW("sdk_basic", "This SDK does not include camera support");
    return !start;
#endif
}
// 与业务采样同属 Worker。只清除 Wi-Fi，保留设备身份和绑定信息。
void reset_wifi() noexcept {
    if (!boot || !application || application->quiescing()) return;
    const auto saved=boot->reset(config::Reset::wifi);
    if ((saved.status==storage::Status::ok || saved.status==storage::Status::unchanged) &&
        saved.applied==storage::Applied::yes && saved.redundant) {
        ESP_LOGI("sdk_basic", "BOOT held 5s: Wi-Fi configuration cleared; restarting for BLE pairing");
        hardware::show({16,0,0});
        hardware::restart();
    } else {
        ESP_LOGW("sdk_basic", "Wi-Fi reset failed; not restarting (status=%d applied=%d redundant=%d)",
                 int(saved.status),int(saved.applied),int(saved.redundant));
    }
}
datapoints::DataPoints current{}, last_reported{};
bool first_report = true, pending = false;
std::uint64_t last_attempt = 0, last_full = 0;
constexpr std::uint64_t memory_log_interval_ms = 10000;
std::uint64_t last_memory_log = 0;
}
void attach(runtime::Application& value, runtime::Boot& owner) noexcept { application=&value; boot=&owner; }
void request_pairing() noexcept { pairing_requested.store(true); }
void initialize() noexcept {
    current = {};
    button = {};
    online = false;
    pairing_requested.store(false);
    // 在此设置 value_* 的产品默认值。平台上的功能描述不会自动执行硬件业务。
    last_reported = current;
    first_report = true;
    pending = false;
    last_attempt = last_full = 0;
    last_memory_log = 0;
}
bool apply(const datapoints::DataPoints& candidate, const datapoints::Fields& written) noexcept {
    for (std::size_t i = 0; i < written.size(); ++i)
        if (written[i]) ESP_LOGI("sdk_basic", "RX datapoint: %s", datapoints::field_names[i]);
    // start_stream 接到真实摄像头；其余喂食/定时数据点暂为内存示例。
    // 重复设置相同值不重复入队；重连和推流失败的重试由摄像头服务负责。
    if (written[std::size_t(datapoints::Field::dp_start_stream)] &&
        candidate.value_start_stream!=current.value_start_stream &&
        !request_stream(candidate.value_start_stream)) return false;
    return true;
}
void sample(datapoints::DataPoints& candidate, std::uint64_t now) noexcept {
    // 复用 Worker 的周期回调，离线和推流期间也持续记录，不额外创建任务。
    if (now - last_memory_log >= memory_log_interval_ms) {
        last_memory_log = now;
        diagnostics::log_memory("runtime");
    }
    if (application) {
        if (!hardware_attempted) {
            hardware_attempted=true; hardware_ready=hardware::initialize();
        }
        if (pairing_requested.exchange(false)) {
            const auto result = application->onboarding().request_mode(onboarding::Mode::ble, now);
            ESP_LOGI("sdk_basic", "BLE pairing request=%d", int(result));
        }
        if (hardware_ready) {
            const auto event=button.update(hardware::pressed(), now);
            if (event==ui::ButtonEvent::long_press) reset_wifi();
            else if (event==ui::ButtonEvent::short_press) {
                const bool start=!candidate.value_start_stream;
                if (request_stream(start)) candidate.value_start_stream=start;
            }
        }
        bool streaming=false;
#ifdef GAGENT_CAMERA_ENABLED
        streaming=camera::service().status().state==camera::State::streaming;
#endif
        const bool ready = application->online();
        if (hardware_ready) hardware::show(ui::indicator(ready,
            application->onboarding().is_open(now), candidate.value_start_stream, streaming, now));
        if (online != ready) {
            online = ready;
            ESP_LOGI("sdk_basic", "IoT MQTT %s", online ? "ready" : "offline");
        }
    }
    // 在此读取传感器或消费其他任务投递的事件，写入 candidate.value_*。
    // 不要在其他任务直接修改 current，也不要在这里 sleep 或等待网络响应。
}
protocol::Status control(protocol::Bytes input, std::uint8_t* output,
                         std::size_t capacity, std::size_t& written) noexcept {
    written = 0;
    if (!input.data || !input.size) return protocol::Status::malformed;
    if (input.data[0] == datapoints::read_action) return datapoints::read_status(input, current, output, capacity, written);
    if (input.data[0] != datapoints::write_action) return protocol::Status::unsupported;
    // 先解码到临时状态，整条请求有效且硬件接受后才提交，避免半包修改状态。
    auto candidate = current;
    datapoints::Fields fields{};
    const auto status = datapoints::decode_control({input.data+1, input.size-1}, candidate, fields);
    if (status != protocol::Status::ok) return status;
    if (!product::apply(candidate, fields)) return protocol::Status::handler_error;
    current = candidate;
    pending = true;
    // 控制成功按协议返回空业务载荷；ACK、请求序号和鉴权由 SDK 路由层处理。
    return protocol::Status::ok;
}
void tick(reports::Service& reports, std::uint64_t now) noexcept {
    auto candidate = current;
    sample(candidate, now);
    if (datapoints::valid(candidate)) current = candidate;
    // 最多每秒尝试上报一次，首次及每 10 秒发送全部字段。
    if (last_attempt && now-last_attempt < 1000) return;
    auto fields = datapoints::changed(last_reported, current);
    const bool full = first_report || now-last_full >= 10000;
    if (full) fields = datapoints::all_fields();
    if (!full && !pending && std::none_of(fields.begin(), fields.end(), [](bool value) {return value;})) return;
    if (pending) fields = datapoints::all_fields();
    std::array<std::uint8_t, datapoints::max_payload> payload{};
    std::size_t size = 0;
    last_attempt = now;
    if (datapoints::encode_status(current, fields, datapoints::report_action, payload.data(), payload.size(), size) != protocol::Status::ok) return;
    if (reports.publish({payload.data(), size})) {
        ESP_LOGI("sdk_basic", "TX state bytes=%u submitted (confirm receipt in App)", unsigned(size));
        // 返回 true 表示已尝试分发，不代表 App 已收到。
        // 定时全量上报和状态查询用于让重新连接的客户端恢复当前状态。
        last_reported = current;
        pending = first_report = false;
        if (full) last_full = now;
    }
}
}
