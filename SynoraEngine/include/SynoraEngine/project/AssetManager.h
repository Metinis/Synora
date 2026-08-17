#pragma once

#include <typeindex>

#include "AssetPool.h"

namespace SYN {
class AssetManager {
  public:
    using CleanupFn = std::function<void(UUID)>;

  public:
    AssetManager() = default;
    ~AssetManager() = default;

    template <typename PoolT> void registerPoolName(std::string_view poolName) {
        getPool<PoolT>().registerName(poolName);
    }

    template <typename AssetT> UUID add(AssetT asset) {
        UUID id = generateUUID();
        getPool<AssetT>().add(id, std::move(asset));
        m_Owner[id] = std::type_index(typeid(AssetT));
        return id;
    }

    template <typename AssetT> const AssetT *get(UUID id) const {
        const AssetPool<AssetT> *pool = findPool<AssetT>();
        if (pool == nullptr)
            return nullptr;
        return pool->getRef(id);
    }
    template <typename AssetT> AssetT *getMut(UUID id) {
        AssetPool<AssetT> *pool = findPool<AssetT>();
        if (pool == nullptr)
            return nullptr;
        return pool->getRefMut(id);
    }

    void addRef(UUID id);
    void removeRef(UUID id);

    void registerCleanup(CleanupFn fn);

    // Notify observers (e.g. render backends) that asset UUID is no longer
    // used.
    void resolvePendingDeletions();

  private:
    template <typename PoolT> AssetPool<PoolT> &getPool() {
        auto poolType = std::type_index(typeid(PoolT));
        if (m_Pools.find(poolType) == m_Pools.cend()) {
            m_Pools.insert(
                std::make_pair(poolType, std::make_unique<AssetPool<PoolT>>()));
        }
        return static_cast<AssetPool<PoolT> &>(*m_Pools.at(poolType));
    }

    template <typename PoolT> const AssetPool<PoolT> *findPool() const {
        auto poolType = std::type_index(typeid(PoolT));
        if (m_Pools.find(poolType) == m_Pools.cend())
            return nullptr;
        return static_cast<const AssetPool<PoolT> *>(
            m_Pools.at(poolType).get());
    }

    template <typename PoolT> AssetPool<PoolT> *findPool() {
        auto poolType = std::type_index(typeid(PoolT));
        if (m_Pools.find(poolType) == m_Pools.cend())
            return nullptr;
        return static_cast<AssetPool<PoolT> *>(m_Pools.at(poolType).get());
    }

  private:
    std::unordered_map<UUID, std::type_index> m_Owner;
    std::unordered_map<UUID, size_t> m_AssetRefs;

    std::vector<UUID> m_PendingDelete;
    std::vector<CleanupFn> m_CleanupObservers;

    std::unordered_map<std::type_index, std::unique_ptr<IAssetPool>> m_Pools;

    std::unordered_map<std::string, UUID> m_LoadedUUIDMap{};
};
} // namespace SYN
