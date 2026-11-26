#pragma once
#include "gagent/protocol/mcu.hpp"
#include "gagent/application/startup.hpp"
namespace gagent::mcu {
enum class HandshakeFormat {unknown,fixed_fields,key_value};
struct ProductField {std::string_view wire,saved;};
// Credential interpretation belongs to the selected provider, not the UART codec.
struct ProductPolicy {
    HandshakeFormat format=HandshakeFormat::fixed_fields;
    const ProductField* fields=nullptr;std::size_t field_count=0;
    const runtime::IdentityScope* identity=nullptr;
    bool cached_handshake=true;
    runtime::ProductTransition transition=nullptr;
};
struct Dialect {
    bool (*product)(protocol::Bytes,std::uint8_t*,std::size_t,std::size_t&) noexcept=nullptr;
    std::uint16_t binary_flag=0;
    HandshakeFormat format=HandshakeFormat::fixed_fields;
};
inline constexpr Dialect fixed_dialect{};
const Dialect& key_value_dialect() noexcept;
inline const Dialect& default_dialect() noexcept {return fixed_dialect;}
}
