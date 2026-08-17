#include "SynoraEngine/project/AssetManager.h"

namespace SYN {

AssetRef AssetManager::acquire(UUID id) {
    if (m_Owner.find(id) == m_Owner.cend())
        return {};
    return AssetRef(this, id);
}

void AssetManager::addRef(UUID id) { ++m_AssetRefs[id]; }
void AssetManager::removeRef(UUID id) {
    if (m_AssetRefs[id] > 0 && --m_AssetRefs[id] == 0) {
        m_PendingDelete.push_back(id);
    }
}

void AssetManager::registerCleanup(CleanupFn fn) {
    m_CleanupObservers.emplace_back(std::move(fn));
}

void AssetManager::resolvePendingDeletions() {
    for (UUID id : m_PendingDelete) {

        auto it = m_AssetRefs.find(id);

        if (it == m_AssetRefs.cend() || it->second > 0)
            continue;

        for (auto &fn : m_CleanupObservers)
            fn(id);

        auto poolType = m_Owner.at(id);
        m_Pools.at(poolType)->remove(id);

        m_AssetRefs.erase(id);
        m_Owner.erase(id);
    }
    m_PendingDelete.clear();
}

} // namespace SYN
