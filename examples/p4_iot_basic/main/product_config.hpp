#pragma once
// PK/PS 仅在 Git 忽略的 product_secrets.hpp 中配置，公开源码不提供默认凭据。
#if __has_include("product_secrets.hpp")
#include "product_secrets.hpp"
#endif
#ifndef GAGENT_SOC_PRODUCT_KEY
#define GAGENT_SOC_PRODUCT_KEY ""
#endif
#ifndef GAGENT_SOC_PRODUCT_SECRET
#define GAGENT_SOC_PRODUCT_SECRET ""
#endif
static_assert(sizeof(GAGENT_SOC_PRODUCT_KEY) == 33,
              "Set your 32-character PK in main/product_secrets.hpp (copy product_secrets.example.hpp).");
static_assert(sizeof(GAGENT_SOC_PRODUCT_SECRET) == 33,
              "Set your 32-character PS in main/product_secrets.hpp (copy product_secrets.example.hpp).");
namespace gagent::board::configuration {
inline constexpr bool enabled = true;
inline constexpr bool allow_migration = false;
inline constexpr char product_secret[] = GAGENT_SOC_PRODUCT_SECRET;
inline constexpr char cloud_host[] = "api.gizwits.com";
inline constexpr unsigned short cloud_port = 80;
inline constexpr char bluetooth_name[] = "GAgent-p4_sdk_basic";
// SoC 产品属性位，与 SDK 约定一致；此示例没有网关和 Beacon 能力。
inline constexpr char attributes[] = "0000000000000002";
}
