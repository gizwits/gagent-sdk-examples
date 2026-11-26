#pragma once
#include "gagent/application/restart.hpp"
namespace gagent::platform {
class EspRestart final : public runtime::Restart {
public:
    void restart() noexcept override;
};
}
