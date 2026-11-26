#pragma once
#include "gagent/ota/ota_health.hpp"
namespace gagent::platform {
class EspBootConfirmation final : public ota::BootConfirmation {
public:
    ota::BootImageState image_state() noexcept override;
    bool confirm() noexcept override;
    bool rollback() noexcept override;
};
}
