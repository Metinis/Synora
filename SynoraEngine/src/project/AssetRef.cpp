#include <SynoraEngine/project/AssetManager.h>
#include <SynoraEngine/project/AssetRef.h>

namespace SYN {
AssetRef::AssetRef(AssetManager *assetManager, UUID id) {
    m_AssetManager = assetManager;
    m_UUID = id;

    m_AssetManager->addRef(m_UUID);
}

AssetRef::AssetRef(const AssetRef &other) {
    m_AssetManager = other.m_AssetManager;
    m_UUID = other.m_UUID;

    if (m_AssetManager != nullptr)
        m_AssetManager->addRef(m_UUID);
}

AssetRef::AssetRef(AssetRef &&other) noexcept {
    m_AssetManager = other.m_AssetManager;
    m_UUID = other.m_UUID;

    other.m_AssetManager = nullptr;
    other.m_UUID = 0;
}

AssetRef &AssetRef::operator=(AssetRef other) noexcept {
    std::swap(m_AssetManager, other.m_AssetManager);
    std::swap(m_UUID, other.m_UUID);
    return *this;
}

AssetRef::~AssetRef() {
    if (m_AssetManager == nullptr)
        return;
    m_AssetManager->removeRef(m_UUID);
}

UUID AssetRef::uuid() const { return m_UUID; }
bool AssetRef::valid() const { return m_AssetManager != nullptr; }
} // namespace SYN
