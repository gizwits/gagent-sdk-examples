#pragma once
#include <cstdint>
namespace gagent::board::ui {
struct Color {
    std::uint8_t red=0, green=0, blue=0;
    constexpr bool operator==(Color other) const noexcept {
        return red==other.red && green==other.green && blue==other.blue;
    }
};
enum class ButtonEvent { none, short_press, long_press };
// 短按在松开后触发；长按满 5 秒立即触发且不再产生短按事件。
// 上电时若一直按住 BOOT，必须先松开，避免重启后连续重置。
class Button {
public:
    ButtonEvent update(bool pressed, std::uint64_t now) noexcept {
        if (!initialized_) {
            initialized_=true; raw_=stable_=pressed; changed_=now;
            armed_=!pressed;
            return ButtonEvent::none;
        }
        if (pressed!=raw_) {raw_=pressed; changed_=now;}
        if (raw_!=stable_ && now-changed_>=30) {
            stable_=raw_;
            if (stable_) {
                active_=armed_; armed_=false; long_sent_=false;
                pressed_at_=changed_;
            } else {
                armed_=true;
                const bool released=active_ && !long_sent_;
                active_=false;
                // 用真实松开边沿计时，不把松开消抖的 30 ms 算入长按。
                if (released) return changed_-pressed_at_>=5000 ? ButtonEvent::long_press : ButtonEvent::short_press;
            }
        }
        if (active_ && stable_ && raw_ && !long_sent_ && now-pressed_at_>=5000) {
            long_sent_=true;
            return ButtonEvent::long_press;
        }
        return ButtonEvent::none;
    }
private:
    bool initialized_=false, raw_=false, stable_=false, armed_=false;
    bool active_=false, long_sent_=false;
    std::uint64_t changed_=0, pressed_at_=0;
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
