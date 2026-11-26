#pragma once
#include "gizwits_protocol.hpp"
#include "gagent/protocol/reports.hpp"
namespace gagent::board::product {
// All functions run on the existing GAgent Worker. Hardware callbacks/other tasks
// must hand off their events through a bounded queue consumed by sample().
void initialize() noexcept;
protocol::Status control(protocol::Bytes, std::uint8_t*, std::size_t, std::size_t&) noexcept;
void tick(reports::Service&, std::uint64_t now) noexcept;
// Product-owned hooks. Keep nonblocking; reject before hardware side effects.
bool apply(const datapoints::DataPoints& candidate, const datapoints::Fields& written) noexcept;
void sample(datapoints::DataPoints& candidate, std::uint64_t now) noexcept;
}
