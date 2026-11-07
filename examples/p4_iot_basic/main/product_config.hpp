#pragma once
// 本示例经授权内置测试产品凭据，方便直接上板验证。
// 正式产品请更换 PK/PS；PK 由数据点协议生成，PS 在这里配置。
// 可使用本地 product_secrets.hpp 覆盖 PS，该文件不纳入 Git。
// 可选本地覆盖：在 product_secrets.hpp 中定义 GAGENT_SOC_PRODUCT_SECRET。
#if __has_include("product_secrets.hpp")
#include "product_secrets.hpp"
#endif
#ifndef GAGENT_SOC_PRODUCT_SECRET
#define GAGENT_SOC_PRODUCT_SECRET "00000000000000000000000000000000"
#endif
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
