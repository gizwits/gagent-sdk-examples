#pragma once
#include "gagent/protocol/protocol.hpp"
namespace gagent::platform {
// Legacy ECB padding: pad partial blocks only; aligned input receives no extra block.
bool encrypt_legacy_ecb(protocol::Bytes,const std::uint8_t* key,std::uint8_t*,std::size_t,std::size_t&) noexcept;
bool decrypt_legacy_ecb_raw(protocol::Bytes,const std::uint8_t* key,std::uint8_t*,std::size_t,std::size_t&) noexcept;
}
