#pragma once
#include "gagent/diagnostics/log.hpp"
#include "gagent/protocol/protocol.hpp"
#include "gagent/protocol/mcu.hpp"

namespace gagent::diagnostics {
inline void mcu_summary(const char* direction,std::uint8_t command,std::uint8_t sequence,
                        std::uint16_t flags,std::size_t payload_size) noexcept {
#if defined(ESP_PLATFORM) && (!defined(CONFIG_GAGENT_MCU_DEBUG) || !CONFIG_GAGENT_MCU_DEBUG)
    GAGENT_LOGI("gagent_mcu","%s command=0x%02x seq=%u payload_bytes=%u flags=0x%04x",
        direction,unsigned(command),unsigned(sequence),unsigned(payload_size),unsigned(flags));
#else
    (void)direction;(void)command;(void)sequence;(void)flags;(void)payload_size;
#endif
}
// TX is already encoded/validated. Decode only the six-byte header without
// allocating another maximum-frame decoder on the task stack.
inline void mcu_tx_summary(protocol::Bytes wire) noexcept {
#if defined(ESP_PLATFORM)
    if(!wire.data || wire.size<9 || wire.data[0]!=255 || wire.data[1]!=255)return;
    // Read only the fixed header and Wi-Fi MAC field. No payload or credentials
    // are formatted, even when the Aliyun information response appends Wi-Fi.
    std::uint8_t decoded[47]{};std::size_t used=0;
    for(std::size_t i=2;i<wire.size && used<sizeof(decoded);++i){
        const auto byte=wire.data[i];
        if(byte==255 && (++i>=wire.size || wire.data[i]!=0x55))return;
        decoded[used++]=byte;
    }
    if(used<6)return;
    const auto length=std::size_t(decoded[0])*256+decoded[1];
    if(length<5 || length-5>mcu::payload_limit || wire.size<length+4)return;
    if(decoded[2]==0x22 && length>=46 && used==sizeof(decoded) && decoded[6]==1){
        char mac[13]{};bool valid=true,zero=true,broadcast=true;
        for(unsigned i=0;i<12;++i){
            auto c=decoded[31+i];
            if(c>='a' && c<='f')c=std::uint8_t(c-'a'+'A');
            valid=valid && ((c>='0' && c<='9') || (c>='A' && c<='F'));
            zero=zero && c=='0';broadcast=broadcast && c=='F';mac[i]=char(c);
        }
        if(valid && !zero && !broadcast){
            GAGENT_LOGI("gagent_mcu","TX command=0x22 seq=%u payload_bytes=%u flags=0x%04x mac=%s",
                unsigned(decoded[3]),unsigned(length-5),unsigned(unsigned(decoded[4])*256+decoded[5]),mac);
            return;
        }
    }
    mcu_summary("TX",decoded[2],decoded[3],std::uint16_t(unsigned(decoded[4])*256+decoded[5]),length-5);
#else
    (void)wire;
#endif
}
// Wire chunks, not decoded payloads: preserve escaping, CRC and partial frames.
// Compile out formatting entirely when the board has not opted in.
inline void mcu_debug(int uart, const char* direction, protocol::Bytes bytes) noexcept {
#if defined(ESP_PLATFORM) && defined(CONFIG_GAGENT_MCU_DEBUG) && CONFIG_GAGENT_MCU_DEBUG
    if(!bytes.data || !bytes.size)return;
    GAGENT_LOGI("gagent_mcu", "UART%d %s raw begin bytes=%u", uart, direction, unsigned(bytes.size));
    constexpr char digits[]="0123456789ABCDEF";
    for(std::size_t offset=0;offset<bytes.size;offset+=16){
        char hex[48]{},ascii[17]{};
        const auto count=bytes.size-offset<16?bytes.size-offset:16;
        for(std::size_t i=0;i<count;++i){
            const auto byte=bytes.data[offset+i];
            hex[i*3]=digits[byte>>4];hex[i*3+1]=digits[byte&15];hex[i*3+2]=' ';
            ascii[i]=byte>=32 && byte<=126?char(byte):'.';
        }
        hex[count*3-1]='\0';
        GAGENT_LOGI("gagent_mcu", "UART%d %s raw offset=%04u hex=%s ascii=%s",
                    uart, direction, unsigned(offset), hex, ascii);
    }
    GAGENT_LOGI("gagent_mcu", "UART%d %s raw end bytes=%u", uart, direction, unsigned(bytes.size));
#else
    (void)uart;(void)direction;(void)bytes;
#endif
}
}
