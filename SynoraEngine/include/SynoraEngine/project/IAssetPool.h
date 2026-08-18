#pragma once

#include "UUID.h"

namespace SYN {
class IAssetPool {
  public:
    virtual ~IAssetPool() = default;
    virtual void remove(UUID id) = 0;

    virtual void serialize(UUID id) = 0;
    virtual void deserialize(UUID id) = 0;
};
} // namespace SYN
