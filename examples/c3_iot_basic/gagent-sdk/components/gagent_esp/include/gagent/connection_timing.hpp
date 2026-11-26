#pragma once
#include <cstdint>
namespace gagent::platform {
// Same SDK task as BEFORE_CONNECT/CONNECTED. No shared clock state across clients.
std::int64_t transport_finished_us() noexcept;
}
