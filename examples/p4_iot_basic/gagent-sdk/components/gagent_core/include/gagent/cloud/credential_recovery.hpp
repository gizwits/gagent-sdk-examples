#pragma once
#include "gagent/storage/boot.hpp"
#include "gagent/cloud/identity.hpp"
namespace gagent::identity {
enum class Renewal {ready,pending_cleanup,invalid_identity,entropy_failure,persistence_failure};
}
