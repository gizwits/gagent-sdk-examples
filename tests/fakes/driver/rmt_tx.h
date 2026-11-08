#pragma once
#include "rmt_encoder.h"
#include <cstddef>
struct FakeChannel{};
using rmt_channel_handle_t=FakeChannel*;
union rmt_symbol_word_t {
    struct {std::uint32_t duration0:15,level0:1,duration1:15,level1:1;};
    std::uint32_t val;
};
constexpr int RMT_CLK_SRC_DEFAULT=0;
struct rmt_tx_channel_config_t {gpio_num_t gpio_num;int clk_src;std::uint32_t resolution_hz;std::size_t mem_block_symbols,trans_queue_depth;};
struct rmt_transmit_config_t {int loop_count;struct {unsigned eot_level:1,queue_nonblocking:1;} flags;};
esp_err_t rmt_new_tx_channel(const rmt_tx_channel_config_t*,rmt_channel_handle_t*);
esp_err_t rmt_enable(rmt_channel_handle_t);
esp_err_t rmt_del_channel(rmt_channel_handle_t);
esp_err_t rmt_transmit(rmt_channel_handle_t,rmt_encoder_handle_t,const void*,std::size_t,const rmt_transmit_config_t*);
esp_err_t rmt_tx_wait_all_done(rmt_channel_handle_t,int);
