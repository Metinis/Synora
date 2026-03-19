#pragma once
#include "UUID.h"
#include "PuzzleEngine/scene/Components.h"
#include "spdlog/spdlog.h"

namespace SYN {
    //todo add more variants here as needed
    using AssetType = std::variant<
        MeshComp,
        MaterialComp
    >;
    template<typename T>
    struct is_variant : std::false_type {};

    template<typename... Ts>
    struct is_variant<std::variant<Ts...>> : std::true_type {};

    template<typename T>
    inline constexpr bool is_variant_v = is_variant<T>::value;

    template<typename T>
    concept isAsset = requires() {is_variant_v<T>;};

    class AssetManager {
    public:
        AssetManager() = default;

        //automatically gen ID
        template<typename T>
        requires isAsset<T>
        UUID addAsset(T asset) {
            UUID id = generateUUID();
            m_AssetMap[id] = asset;
            spdlog::debug("Asset added: %d", id);
            return id;
        }

        //to be used when we manually gen ID
        //todo check if mesh, if so, create gpu equivalent
        template<typename T>
        requires isAsset<T>
        void createAsset(T asset, UUID id) {
            m_AssetMap[id] = asset;
            spdlog::debug("Asset created: %d", id);
        }

        template<typename T>
        T* get(UUID id) {
            if (m_AssetMap.contains(id)) {
                return m_AssetMap[id];
            }
            spdlog::error("Asset not found!: %d", id);
            return nullptr;
        }

        ~AssetManager() = default;
    private:
        std::unordered_map<UUID, AssetType> m_AssetMap{};
    };
}
