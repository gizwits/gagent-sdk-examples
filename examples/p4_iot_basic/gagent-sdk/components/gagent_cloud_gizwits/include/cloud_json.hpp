#pragma once
#include "gagent/cloud/cloud.hpp"
#include "gagent/ota/ota_check.hpp"
namespace gagent::platform {
bool ota_check_json(std::string_view,ota::CheckReply&) noexcept;
bool cloud_json(cloud::Operation,std::string_view,cloud::Response&) noexcept;
}
