#pragma once
#include "gagent/protocol/protocol.hpp"
namespace gagent::local {
class ResetControl {
public:
    virtual ~ResetControl()=default;
    virtual protocol::Status reset(const protocol::Context&,const protocol::Frame&,protocol::Reply&,std::uint64_t now) noexcept=0;
    virtual bool resetting() const noexcept=0;
};
}
