#pragma once
#include "gagent/protocol/mcu_dialect.hpp"
namespace gagent::mcu {
enum class HandshakeError {none,malformed,invalid_product,ambiguous,format_mismatch};
// Owned, bounded product information. The normalized storage layout is private to
// MCU compatibility; providers receive the untouched key/value credentials too.
struct HandshakeInfo {
    std::array<std::uint8_t,payload_limit> bytes{};
    std::size_t size=0;
    HandshakeFormat format=HandshakeFormat::unknown;
    protocol::Bytes wire() const noexcept {return {bytes.data(),size};}
    std::string_view version(unsigned index) const noexcept {
        if(index>=4 || size<32)return {};
        const auto* p=reinterpret_cast<const char*>(bytes.data()+index*8);
        std::size_t n=0;while(n<8 && p[n])++n;return {p,n};
    }
    protocol::Bytes attributes() const noexcept {return size>=74?protocol::Bytes{bytes.data()+66,8}:protocol::Bytes{};}
    std::string_view extension(std::string_view key) const noexcept {
        if(size<108 || size>bytes.size() || std::size_t(bytes[106])*256+bytes[107]!=size-108)return {};
        auto trim=[](std::string_view value){
            while(!value.empty() && value.front()==' ')value.remove_prefix(1);
            while(!value.empty() && value.back()==' ')value.remove_suffix(1);
            return value;
        };
        std::string_view text(reinterpret_cast<const char*>(bytes.data()+108),size-108);
        if(!text.empty() && text.back()=='\0')text.remove_suffix(1);
        while(!text.empty()){
            const auto end=text.find_first_of("&\n");auto entry=text.substr(0,end);
            if(!entry.empty() && entry.back()=='\r')entry.remove_suffix(1);
            const auto equal=entry.find('=');
            if(equal!=entry.npos && trim(entry.substr(0,equal))==key)return trim(entry.substr(equal+1));
            if(end==text.npos)break;
            text.remove_prefix(end+1);
        }
        return {};
    }
    // Only the current accepted handshake can opt in. Cached fixed identity has
    // no extensions and therefore cannot silently enable streaming after reboot.
    bool ota_streaming() const noexcept {return extension("otaStream")=="1";}
    // Single-turn is the default, including cached identities and invalid values.
    bool continuous_dialogue() const noexcept {return extension("dialogueMode")=="1";}
    std::uint16_t binding_seconds() const noexcept {return size>=66?std::uint16_t(unsigned(bytes[64])*256+bytes[65]):0;}
};
// No I/O, storage or cloud selection. On failure output has no valid product.
HandshakeError parse_handshake(protocol::Bytes,HandshakeInfo&) noexcept;
}
