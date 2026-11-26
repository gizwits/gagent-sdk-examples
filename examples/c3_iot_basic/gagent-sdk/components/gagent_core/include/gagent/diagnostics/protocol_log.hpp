#pragma once
#include "gagent/diagnostics/log.hpp"
#include "gagent/protocol/protocol.hpp"
namespace gagent::diagnostics {
constexpr const char* source(protocol::Channel channel) noexcept {
    switch(channel){
    case protocol::Channel::lan:return "LAN";
    case protocol::Channel::mqtt:return "MQTT";
    case protocol::Channel::ble:return "BLE";
    }
    return "UNKNOWN";
}
constexpr const char* status(protocol::Status value) noexcept {
    switch(value){
    case protocol::Status::ok:return "ok";
    case protocol::Status::need_more:return "need_more";
    case protocol::Status::malformed:return "malformed";
    case protocol::Status::unsupported:return "unsupported";
    case protocol::Status::too_large:return "too_large";
    case protocol::Status::unauthorized:return "unauthorized";
    case protocol::Status::wrong_device:return "wrong_device";
    case protocol::Status::handler_error:return "handler_error";
    }
    return "unknown";
}
}
