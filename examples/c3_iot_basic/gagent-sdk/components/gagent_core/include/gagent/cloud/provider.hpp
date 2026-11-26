#pragma once
#include "gagent/cloud/mqtt.hpp"
#include "gagent/provisioning/onboarding.hpp"
#include "gagent/network/ble.hpp"
namespace gagent::cloud {
struct Dependencies {storage::Md5 md5;Cipher encrypt,decrypt;identity::Random random;};
// Immutable provider-owned policy. All callbacks run on the application worker;
// they stage data only. Boot remains the sole owner of durable transactions.
struct ProviderPolicy {
    std::string_view id;
    mcu::ProductPolicy mcu_product;
    config::ResetTransform reset=nullptr;
    ble::ProductEncoder advertisement=nullptr;
    std::string_view ota_product_key;
    bool owns_registration_flag=false;
    void (*configure_onboarding)(onboarding::Policy&) noexcept=nullptr;
    bool ota_hardware_identity=true; // Whether hardware distinguishes OTA attempt budgets.
};
// Protocols and policy outlive the application. No hidden provider selection in core.
struct Provider {HttpProtocol& http;mqtt::Protocol& mqtt;const ProviderPolicy& policy;};
}
