#include "hardware.hpp"
#include "driver/rmt_tx.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace gagent::board;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"%d: %s\n",__LINE__,#x);std::abort();}} while(0)
FakeChannel channel;FakeEncoder encoder;
int failure=0,level=1,configs=0,channels=0,deleted_channels=0,deleted_encoders=0,transmits=0;
bool busy=false;
void esp_restart(){std::abort();} // 本测试不执行设备重启。
const rmt_symbol_word_t* borrowed=nullptr;
std::size_t symbol_count=0;
esp_err_t gpio_config(const gpio_config_t* c){CHECK(c->pin_bit_mask==(1ULL<<35) && c->mode==GPIO_MODE_INPUT && c->pull_up_en==1 && c->intr_type==0);++configs;return failure==1?ESP_FAIL:ESP_OK;}
int gpio_get_level(gpio_num_t pin){CHECK(pin==GPIO_NUM_35);return level;}
const char* esp_err_to_name(esp_err_t){return "fake";}
esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t* c,rmt_channel_handle_t* out){CHECK(c->gpio_num==GPIO_NUM_51 && c->resolution_hz==10000000 && c->mem_block_symbols==64 && c->trans_queue_depth==1);if(failure==2)return ESP_FAIL;*out=&channel;++channels;return ESP_OK;}
esp_err_t rmt_new_copy_encoder(const rmt_copy_encoder_config_t*,rmt_encoder_handle_t* out){if(failure==3)return ESP_FAIL;*out=&encoder;return ESP_OK;}
esp_err_t rmt_enable(rmt_channel_handle_t c){CHECK(c==&channel);return failure==4?ESP_FAIL:ESP_OK;}
esp_err_t rmt_del_channel(rmt_channel_handle_t c){CHECK(c==&channel);++deleted_channels;return ESP_OK;}
esp_err_t rmt_del_encoder(rmt_encoder_handle_t e){CHECK(e==&encoder);++deleted_encoders;return ESP_OK;}
esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t c,int ms){CHECK(c==&channel && ms==0);return busy?ESP_FAIL:ESP_OK;}
esp_err_t rmt_transmit(rmt_channel_handle_t c,rmt_encoder_handle_t e,const void* bytes,std::size_t size,const rmt_transmit_config_t* config){
 CHECK(c==&channel && e==&encoder && config->loop_count==0 && config->flags.queue_nonblocking && !config->flags.eot_level);
 if(failure==5)return ESP_FAIL;
 CHECK(!busy);borrowed=static_cast<const rmt_symbol_word_t*>(bytes);symbol_count=size/sizeof(rmt_symbol_word_t);busy=true;++transmits;return ESP_OK;
}
int main(){
 CHECK(!hardware::pressed() && !hardware::show({}));
 for(int scenario=1;scenario<=4;++scenario){failure=scenario;CHECK(!hardware::initialize());}
 CHECK(channels==2 && deleted_channels==2 && deleted_encoders==1);
 failure=0;CHECK(hardware::initialize());const auto calls=configs;CHECK(hardware::initialize() && configs==calls);
 CHECK(!hardware::pressed());level=0;CHECK(hardware::pressed());
 CHECK(hardware::show({1,2,4}) && symbol_count==25 && transmits==1);
 const std::uint8_t expected[]={2,1,4};
 for(unsigned byte=0;byte<3;++byte)for(unsigned bit=0;bit<8;++bit){const auto& s=borrowed[byte*8+bit];const bool one=expected[byte]&(0x80>>bit);CHECK(s.level0==1 && !s.level1 && s.duration0==(one?9u:3u) && s.duration1==(one?3u:9u));}
 CHECK(!borrowed[24].level0 && !borrowed[24].level1 && borrowed[24].duration0+borrowed[24].duration1==3000);
 rmt_symbol_word_t saved[25];std::memcpy(saved,borrowed,sizeof(saved));
 CHECK(!hardware::show({4,5,6}) && transmits==1 && !std::memcmp(saved,borrowed,sizeof(saved)));
 busy=false;CHECK(hardware::show({1,2,4}) && transmits==1); // Coalesce same completed color.
 failure=5;CHECK(!hardware::show({4,5,6}));failure=0;CHECK(hardware::show({4,5,6}) && transmits==2);
 std::puts("P4 hardware: pin configuration, WS2812 GRB/timing, async payload lifetime and init failure cleanup passed");
}
