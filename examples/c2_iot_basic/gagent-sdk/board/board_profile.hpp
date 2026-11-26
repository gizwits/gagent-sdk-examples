#pragma once
#include "gagent/application/board.hpp"
namespace gagent::board {
inline constexpr Profile selected{Mode::soc, false, false, 115200, 55000, 600, true, true};
static_assert(valid(selected), "Invalid board profile");
}
