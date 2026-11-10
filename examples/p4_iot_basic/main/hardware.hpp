#pragma once
#include "board_ui.hpp"
namespace gagent::board::hardware {
// WT9932P4C61-TINY：BOOT=GPIO35（低电平按下），WS2812=GPIO51（GRB）。
// 仅由 Worker 调用；首次 sample 时初始化，避免影响启动阶段的熵源初始化。
bool initialize() noexcept;
bool pressed() noexcept;
// 仅在配置双备份已确认提交后调用。
void restart() noexcept;
// 非阻塞刷新；RMT 完成前保留静态发送缓冲区，不创建额外任务。
bool show(ui::Color color) noexcept;
}
