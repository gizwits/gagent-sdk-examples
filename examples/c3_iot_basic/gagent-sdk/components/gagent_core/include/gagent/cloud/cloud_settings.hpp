#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/protocol/protocol.hpp"
namespace gagent::cloud {
// Owned values survive the MQTT event buffer being reused before the worker commits.
struct Settings {
    std::array<char,61> server{};
    std::array<char,6> port{};
    std::array<char,7> timezone{};
    bool operator==(const Settings& other) const noexcept {
        return server==other.server && port==other.port && timezone==other.timezone;
    }
    storage::WriteResult apply(runtime::Boot&) const noexcept;
};
protocol::Status settings(std::string_view,Settings&) noexcept;
}
