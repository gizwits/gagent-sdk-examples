#include "hardware.hpp"
#include "driver/gpio.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "esp_log.h"
#include "esp_system.h"
#include <array>
namespace gagent::board::hardware {
namespace {
constexpr gpio_num_t button_pin=GPIO_NUM_35, led_pin=GPIO_NUM_51;
rmt_channel_handle_t channel=nullptr;
rmt_encoder_handle_t encoder=nullptr;
std::array<rmt_symbol_word_t,25> symbols{};
ui::Color previous{};
bool initialized=false,in_flight=false,have_color=false;
}
bool initialize() noexcept {
    if(initialized)return true;
    gpio_config_t button{};
    button.pin_bit_mask=1ULL<<button_pin;
    button.mode=GPIO_MODE_INPUT;
    button.pull_up_en=GPIO_PULLUP_ENABLE;
    button.pull_down_en=GPIO_PULLDOWN_DISABLE;
    button.intr_type=GPIO_INTR_DISABLE;
    auto result=gpio_config(&button);
    if(result!=ESP_OK){ESP_LOGE("p4-example","BOOT input init failed: %s",esp_err_to_name(result));return false;}
    rmt_tx_channel_config_t tx{};
    tx.gpio_num=led_pin;
    tx.clk_src=RMT_CLK_SRC_DEFAULT;
    tx.resolution_hz=10000000;
    tx.mem_block_symbols=64; // P4 通道：容纳 24 位颜色数据和复位符号。
    tx.trans_queue_depth=1;
    result=rmt_new_tx_channel(&tx,&channel);
    if(result==ESP_OK){rmt_copy_encoder_config_t config{};result=rmt_new_copy_encoder(&config,&encoder);}
    if(result==ESP_OK)result=rmt_enable(channel);
    if(result!=ESP_OK){
        if(encoder){rmt_del_encoder(encoder);encoder=nullptr;}
        if(channel){rmt_del_channel(channel);channel=nullptr;}
        ESP_LOGE("p4-example","RGB LED init failed: %s",esp_err_to_name(result));
        return false;
    }
    initialized=true;
    ESP_LOGI("p4-example","BOOT=GPIO35 active-low; WS2812=GPIO51; short press: toggle streaming; hold 5s: reset Wi-Fi");
    return true;
}
bool pressed() noexcept {return initialized && gpio_get_level(button_pin)==0;}
void restart() noexcept {esp_restart();}
bool show(ui::Color color) noexcept {
    if(!initialized)return false;
    if(in_flight){
        if(rmt_tx_wait_all_done(channel,0)!=ESP_OK)return false;
        in_flight=false;
    }
    if(have_color && color==previous)return true;
    const std::uint8_t bytes[]={color.green,color.red,color.blue};
    unsigned pos=0;
    for(auto byte:bytes)for(int bit=7;bit>=0;--bit){
        auto& symbol=symbols[pos++];
        symbol.val=0;
        symbol.level0=1;symbol.level1=0;
        symbol.duration0=(byte&(1u<<bit))?9:3;
        symbol.duration1=(byte&(1u<<bit))?3:9;
    }
    // 最后保持低电平 300 微秒复位；传输完成前不修改 symbols。
    symbols[24].val=0;symbols[24].duration0=1500;symbols[24].duration1=1500;
    rmt_transmit_config_t config{};
    config.flags.queue_nonblocking=1;
    const auto result=rmt_transmit(channel,encoder,symbols.data(),sizeof(symbols),&config);
    if(result!=ESP_OK)return false;
    previous=color;have_color=in_flight=true;
    return true;
}
}
