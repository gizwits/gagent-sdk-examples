#pragma once
#include "gizwits_protocol.hpp"
#include "gagent/protocol/reports.hpp"
namespace gagent::runtime { class Application; }
namespace gagent::board::product {
// 启动阶段调用，绑定生命周期覆盖整个应用的 SDK 对象。
void attach(runtime::Application&) noexcept;
// 可从普通业务任务调用：仅设置原子标志，实际配网操作由 Worker 完成。
// 中断中应先投递按键事件，不要直接操作 SDK 服务。
void request_pairing() noexcept;
// 启动前设置产品默认状态；不清除已保存的 Wi-Fi 和设备身份。
void initialize() noexcept;
// 以下函数由同一个 GAgent Worker 调用，禁止阻塞等待或再次进入 SDK。
protocol::Status control(protocol::Bytes, std::uint8_t*, std::size_t, std::size_t&) noexcept;
void tick(reports::Service&, std::uint64_t now) noexcept;
// 客户主要实现这两个函数：apply 处理下发，sample 更新需要上报的状态。
// written 指明本次修改的字段；先校验完整请求，再产生硬件副作用。
bool apply(const datapoints::DataPoints& candidate, const datapoints::Fields& written) noexcept;
void sample(datapoints::DataPoints& candidate, std::uint64_t now) noexcept;
}
