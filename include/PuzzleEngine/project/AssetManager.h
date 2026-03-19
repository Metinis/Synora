#pragma once
#include "UUID.h"
#include "PuzzleEngine/core/InputTypes.h"
#include "PuzzleEngine/scene/Components.h"
#include "spdlog/spdlog.h"
#include "Assets.h"
#include "renderer/IBackend.h"
#include "renderer/Renderer.h"
#include "renderer/backends/vulkan/Core.h"

namespace SYN {
    class AssetManager {
    public:
        AssetManager() = default;

        void init(EngineContext* ctx);

        //automatically gen ID
        template<typename T>
        requires isAsset<T>
        UUID addAsset(T asset) {
            UUID id = generateUUID();
            m_AssetMap[id] = asset;
            //generate mesh data if we added one
            spdlog::debug("Asset added: {}", id);
            if constexpr (std::is_same_v<T, MeshData>) {
                m_Renderer->addMesh(id, asset);
            }
            return id;
        }

        //to be used when we manually gen ID
        template<typename T>
        requires isAsset<T>
        void createAsset(T asset, UUID id) {
            m_AssetMap[id] = asset;
            //generate mesh data if we added one
            spdlog::debug("Asset created: %d", id);
            if (std::holds_alternative<MeshData>(asset)) {
                m_Renderer->addMesh(id, std::get<MeshData>(asset));
            }
        }

        template<typename T>
        requires isAsset<T>
        T* get(UUID id) {
            //check if the id is what the user requested
            if (m_AssetMap.contains(id) && std::holds_alternative<T>(m_AssetMap[id])) {
                return m_AssetMap[id];
            }
            spdlog::error("Asset not found, ID!: %d Type: %s", id, typeid(T).name());
            return nullptr;
        }

        ~AssetManager() = default;
    private:
        std::unordered_map<UUID, AssetType> m_AssetMap{};
        Renderer* m_Renderer{};
    };
}
