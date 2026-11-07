#pragma once
#include <cstdint>
namespace gagent::board::ui {
struct Color {
    std::uint8_t red=0, green=0, blue=0;
    constexpr bool operator==(Color other) const noexcept {
        return red==other.red && green==other.green && blue==other.blue;
    }
};
// 独立于 GPIO 的按键消抖逻辑：每次稳定按下只产生一次事件。
// 上电时若一直按住 BOOT，必须先松开，避免意外启动推流。
class Button {
public:
    bool update(bool pressed, std::uint64_t now) noexcept {
        if (!initialized_) {
            initialized_=true; raw_=stable_=pressed; changed_=now;
            armed_=!pressed;
            return false;
        }
        if (pressed!=raw_) {raw_=pressed; changed_=now;}
        if (raw_==stable_ || now-changed_<30) return false;
        stable_=raw_;
        if (!stable_) {armed_=true; return false;}
        const bool event=armed_; armed_=false;
        return event;
    }
private:
    bool initialized_=false, raw_=false, stable_=false, armed_=false;
    std::uint64_t changed_=0;
};
// 配网/掉线优先红闪；启动已接受但尚未连接时蓝闪，实际推流后蓝色常亮。
constexpr Color indicator(bool online, bool pairing, bool requested, bool streaming, std::uint64_t now) noexcept {
    if (pairing || !online) return now%1000<500 ? Color{16,0,0} : Color{};
    if (streaming) return {0,0,16};
    // 包括请求排队、建立连接及失败重试，让按键操作及时获得反馈。
    if (requested) return now%500<250 ? Color{0,0,16} : Color{};
    return {0,16,0};
}
}
