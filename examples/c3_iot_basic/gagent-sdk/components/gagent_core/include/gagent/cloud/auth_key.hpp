#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/cloud/cloud.hpp"
namespace gagent::identity {
using AuthKey=std::array<char,33>;
enum class AuthState {ready,storage_unavailable,invalid_identity,crypto_failure,persistence_failure};
// Legacy compatibility derivation: MD5(lowercase MAC text), then legacy AES-ECB
// of that 12-byte text, lowercase ciphertext hex. Not a source of random entropy.
bool derive_auth_key(const std::array<std::uint8_t,6>&,storage::Md5,cloud::Cipher,AuthKey&) noexcept;
// Only on the configuration owner before activation. Existing values always win.
// Missing values use explicit production input first, otherwise the legacy derivation.
AuthState ensure_auth_key(runtime::Boot&,const std::array<std::uint8_t,6>&,storage::Md5,cloud::Cipher,std::string_view provisioned={}) noexcept;
}
