#pragma once

#include "UUID.h"

namespace SYN {
class AssetManager;
class AssetRef {
  public:
    AssetRef() = default;
    AssetRef(const AssetRef &);
    AssetRef(AssetRef &&) noexcept;
    AssetRef &operator=(AssetRef) noexcept;
    ~AssetRef();

  public:
    UUID uuid() const;
    bool valid() const;

  private:
    AssetRef(AssetManager *assetManager, UUID id);

    friend class AssetManager;
    AssetManager *m_AssetManager = nullptr;
    UUID m_UUID = 0;
};
} // namespace SYN
