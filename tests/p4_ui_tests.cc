#include "board_ui.hpp"
#include <cstdio>
#include <cstdlib>
using namespace gagent::board::ui;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"%d: %s\n",__LINE__,#x); std::abort(); } } while(0)
int main() {
    constexpr auto none=ButtonEvent::none, short_press=ButtonEvent::short_press, long_press=ButtonEvent::long_press;
    Button key;
    CHECK(key.update(true,0)==none); CHECK(key.update(true,10000)==none); // 启动长按不重置
    CHECK(key.update(false,10001)==none); CHECK(key.update(false,10031)==none);
    CHECK(key.update(true,10040)==none); CHECK(key.update(false,10050)==none); // 按下抖动
    CHECK(key.update(true,10060)==none); CHECK(key.update(true,10089)==none);
    CHECK(key.update(true,10090)==none); // 稳定按下也不立即推流
    CHECK(key.update(false,10100)==none); CHECK(key.update(true,10110)==none); // 松开抖动
    CHECK(key.update(false,10120)==none); CHECK(key.update(false,10149)==none);
    CHECK(key.update(false,10150)==short_press); CHECK(key.update(false,11000)==none);
    // 满 5 秒时立即触发一次；持续按住、松开都不能产生推流事件。
    CHECK(key.update(true,12000)==none); CHECK(key.update(true,12030)==none);
    CHECK(key.update(true,16999)==none); CHECK(key.update(true,17000)==long_press);
    CHECK(key.update(true,27000)==none);
    CHECK(key.update(false,27001)==none); CHECK(key.update(false,27031)==none);
    // 长按后仍可再次短按；松开消抖时间不能把不足 5 秒的按压变成长按。
    CHECK(key.update(true,28000)==none); CHECK(key.update(true,28030)==none);
    CHECK(key.update(false,32999)==none); CHECK(key.update(false,33029)==short_press);
    // Worker 恰好错过满 5 秒的采样，松开后仍按实际持续时间判定。
    CHECK(key.update(true,34000)==none); CHECK(key.update(true,34030)==none);
    CHECK(key.update(false,39000)==none); CHECK(key.update(false,39030)==long_press);
    CHECK(key.update(false,40000)==none);
    // 小于消抖时长的按压不算短按。
    CHECK(key.update(true,41000)==none); CHECK(key.update(false,41029)==none);
    CHECK(key.update(false,41059)==none);
    CHECK((indicator(false,false,false,false,0)==Color{16,0,0}));
    CHECK((indicator(false,false,true,true,500)==Color{})); // 掉线即红闪，不残留蓝色
    CHECK((indicator(true,true,true,true,1000)==Color{16,0,0}));
    CHECK((indicator(true,true,true,false,1500)==Color{}));
    CHECK((indicator(true,false,false,false,500)==Color{0,16,0}));
    CHECK((indicator(true,false,true,true,500)==Color{0,0,16}));
    CHECK((indicator(true,false,true,false,0)==Color{0,0,16})); // 启动/重试：蓝闪
    CHECK((indicator(true,false,true,false,250)==Color{}));
    CHECK((indicator(true,false,true,false,500)==Color{0,0,16}));
    CHECK((indicator(true,false,true,true,250)==Color{0,0,16})); // 已连接：不再闪烁
    CHECK((indicator(true,false,false,false,250)==Color{0,16,0})); // 取消等待：恢复绿灯
    CHECK((indicator(false,false,true,false,0)==Color{16,0,0})); // 掉线红闪优先
    std::puts("P4 button: release short press, 5s one-shot reset, startup hold, debounce and LED priority passed");
}
