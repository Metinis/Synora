#pragma once
#include "FileListener.h"
#include "SynoraEngine/core/Layer.h"
#include "SynoraEngine/scene/ISystem.h"

namespace SYN {
struct System {
    void* handle{};
    std::filesystem::path libraryPath{};
    std::unique_ptr<ISystem, void(*)(ISystem*)> system;
};
struct PendingReload {
    std::filesystem::path path;
    float timer;
};
class ScriptManager : public ILayer {
public:
    ScriptManager() = default;

    void init(EngineContext* ctx, const std::filesystem::path& path = "");

    void onAttach() override;
    void onUpdate(float dt) override;
    void onRender() override {};
    void onUIRender() override {};
    void onDetach() override;
    void loadAllSystems();
private:
    bool initAllSystems(const std::filesystem::path& path);
    void loadSystem(ISystem* system);
    void reloadSystem(const std::string& path);
    void unloadSystem(ISystem* system);
    void unloadAllSystems();
    void handleFileChanged(const std::string &path, efsw::Action action);

    std::unordered_map<std::string, System> m_Systems;
    std::vector<PendingReload> m_PendingReloads;
    FileListener m_Listener;
    EngineContext* m_Ctx{};
};
}

