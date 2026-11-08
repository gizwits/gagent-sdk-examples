#include "board_ui.hpp"
#include <cstdio>
#include <cstdlib>
using namespace gagent::board::ui;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"%d: %s\n",__LINE__,#x); std::abort(); } } while(0)
int main() {
    Button key;
    CHECK(!key.update(true,0)); CHECK(!key.update(true,1000)); // 启动按住不触发
    CHECK(!key.update(false,1001)); CHECK(!key.update(false,1031));
    CHECK(!key.update(true,1040)); CHECK(!key.update(false,1050)); // 抖动不算按下
    CHECK(!key.update(true,1060)); CHECK(!key.update(true,1089));
    CHECK(key.update(true,1090)); CHECK(!key.update(true,5000)); // 长按只触发一次
    CHECK(!key.update(false,5001)); CHECK(!key.update(false,5031));
    CHECK(!key.update(true,5040)); CHECK(key.update(true,5070)); // 松开后可再次停止
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
    std::puts("P4 button debounce/boot hold/repeated presses and LED priority passed");
}
