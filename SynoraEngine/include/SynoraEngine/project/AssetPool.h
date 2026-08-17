#pragma once

#include "IAssetPool.h"

namespace SYN {
template <typename AssetT> class AssetPool : public IAssetPool {
  public:
    AssetPool() = default;
    ~AssetPool() = default;

    void registerName(std::string_view poolName) { m_Name = poolName; }

    AssetT *getRefMut(UUID asset) {
        if (m_DataCache.find(asset) == m_DataCache.cend())
            return nullptr;
        return &m_DataCache.at(asset);
    }

    const AssetT *getRef(UUID asset) const {
        if (m_DataCache.find(asset) == m_DataCache.cend())
            return nullptr;
        return &m_DataCache.at(asset);
    }

    void add(UUID id, AssetT &&asset) {
        m_DataCache.emplace(id, std::move(asset));
    }

    void remove(UUID id) override {
        if (m_DataCache.find(id) == m_DataCache.cend())
            return;
        m_DataCache.erase(id);
    }

    // TODO: flesh out serialization callbacks
    // per AssetPool to be serialized
    void serialize(UUID id) override {}
    void deserialize(UUID id) override {}

  private:
    std::unordered_map<UUID, AssetT> m_DataCache;

    // Set for every pool you want to serialize.
    // Assets of this pool are grouped by this label.
    std::optional<std::string> m_Name = std::nullopt;
};
} // namespace SYN
