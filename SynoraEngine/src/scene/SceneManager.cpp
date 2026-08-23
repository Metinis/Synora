#include <SynoraEngine/scene/SceneManager.h>

#include <entt/entt.hpp>

#include <spdlog/spdlog.h>

namespace SYN {

SceneManager::SceneManager() {
    m_Scenes.emplace_back(nullptr);
    m_CurrentGeneration.emplace_back(0);
}

SceneHandle SceneManager::createScene(std::string_view name,
                                      std::optional<uint32_t> priority) {
    if (name.empty()) {
        spdlog::error("Cannot create scene with empty name.");
        return {};
    }

    if (auto nameIt = m_NameToHandle.find(std::string(name));
        nameIt != m_NameToHandle.cend()) {
        spdlog::error(
            "Cannot create scene with name: [{}], as it already exists.", name);
        return {};
    }

    auto updateOrderedScenes = [&](SceneHandle sceneHandle,
                                   std::optional<uint32_t> sceneIndex) {
        if (!sceneIndex.has_value()) {
            m_OrderedSceneList.push_back(std::make_pair(
                sceneHandle, std::numeric_limits<uint32_t>().max()));
        } else {
            m_OrderedSceneList.push_back(
                std::make_pair(sceneHandle, sceneIndex.value()));
            std::stable_sort(
                m_OrderedSceneList.begin(), m_OrderedSceneList.end(),
                [](std::tuple<SceneHandle, uint32_t> a,
                   std::tuple<SceneHandle, uint32_t> b) {
                    return std::get<uint32_t>(a) < std::get<uint32_t>(b);
                });
        }
    };

    if (!m_FreeHandles.empty()) {
        SceneHandle freeHandle = m_FreeHandles.back();
        m_FreeHandles.pop_back();

        freeHandle.generation = m_CurrentGeneration.at(freeHandle.index);

        m_Scenes[freeHandle.index] = std::make_unique<Scene>(name);
        m_NameToHandle[std::string(name)] = freeHandle;

        updateOrderedScenes(freeHandle, priority);

        return freeHandle;
    }

    // There's no way any game gets here. We could store handles with fewer bits
    // but it's a non-issue right now which is why a 64 bit handle (index +
    // generation) is fine.
    if (m_Scenes.size() >= std::numeric_limits<uint32_t>().max()) {
        spdlog::error("Unable to allocate new scene for {}", name);
        return {};
    }

    m_Scenes.emplace_back(std::make_unique<Scene>(name));
    uint32_t newIndex = m_Scenes.size() - 1;
    m_CurrentGeneration.push_back(0);
    SceneHandle handle{newIndex, m_CurrentGeneration[newIndex]};
    m_NameToHandle[std::string(name)] = handle;

    updateOrderedScenes(handle, priority);

    return handle;
}

bool SceneManager::isSceneValid(SceneHandle scene) const {
    if (scene.index == 0)
        return false;

    if (scene.index >= m_CurrentGeneration.size())
        return false;

    if (m_Scenes.at(scene.index) == nullptr)
        return false;

    return scene.generation == m_CurrentGeneration.at(scene.index);
}

bool SceneManager::isSceneActive(SceneHandle scene) const {
    if (!m_CurrentScene.has_value())
        return false;
    return m_CurrentScene.value() == scene;
}

bool SceneManager::isSceneNext(SceneHandle scene) const {
    if (!m_NextScene.has_value())
        return false;
    return m_NextScene.value() == scene;
}

void SceneManager::removeScene(SceneHandle &handle) {
    if (!isSceneValid(handle)) {
        spdlog::error("Cannot remove scene that is already removed.");
        return;
    }
    if (isSceneActive(handle)) {
        spdlog::error("Cannot remove scene that is currently active. Switch to "
                      "another scene first.");
        return;
    }
    if (isSceneNext(handle)) {
        spdlog::error(
            "Cannot remove scene that is set to be the next active scene.");
        return;
    }

    auto orderedListIt =
        std::find_if(m_OrderedSceneList.cbegin(), m_OrderedSceneList.cend(),
                     [&](std::tuple<SceneHandle, uint32_t> pair) {
                         return std::get<SceneHandle>(pair) == handle;
                     });

    if (orderedListIt != m_OrderedSceneList.cend()) {
        m_OrderedSceneList.erase(orderedListIt);
        std::stable_sort(m_OrderedSceneList.begin(), m_OrderedSceneList.end(),
                         [](std::tuple<SceneHandle, uint32_t> a,
                            std::tuple<SceneHandle, uint32_t> b) {
                             return std::get<uint32_t>(a) <
                                    std::get<uint32_t>(b);
                         });
    }

    const std::string &sceneName = m_Scenes.at(handle.index)->getName();
    m_NameToHandle.erase(sceneName);
    m_Scenes.at(handle.index).reset();

    if (m_CurrentGeneration.at(handle.index) >=
        std::numeric_limits<uint32_t>().max()) {
        // How did you even get here?
        spdlog::warn("Reached end of pool for scene slot {}.", handle.index);
        handle.index = 0;
        handle.generation = 0;
        return;
    }

    ++m_CurrentGeneration[handle.index];
    m_FreeHandles.push_back(handle);
    handle.index = 0;
    handle.generation = 0;
}

void SceneManager::nextScene(float delay) {
    SceneHandle nextScene = getNext();
    if (!isSceneValid(nextScene))
        return;
    switchTo(nextScene, delay);
}

void SceneManager::previousScene(float delay) {
    SceneHandle previousScene = getPrevious();
    if (!isSceneValid(previousScene))
        return;
    switchTo(previousScene, delay);
}

SceneHandle SceneManager::getPrevious() const {
    if (m_OrderedSceneList.empty()) {
        spdlog::info("Can't get previous scene because scene list is empty.");
        return {};
    }

    if (!m_CurrentScene.has_value()) {
        spdlog::error("No current scene set. Cannot get previous scene.");
        return {};
    }

    uint32_t i = 0;
    for (i = 0; i < m_OrderedSceneList.size(); ++i) {
        auto [handle, _] = m_OrderedSceneList.at(i);
        if (handle == m_CurrentScene.value())
            break;
    }

    if (i == m_OrderedSceneList.size()) {
        spdlog::critical("Current scene isn't in ordered list.");
        return {};
    }

    if (i < 1) {
        spdlog::error("Already at start of ordered scene list.");
        return {};
    }

    return std::get<SceneHandle>(m_OrderedSceneList.at(i - 1));
}

SceneHandle SceneManager::getNext() const {
    if (m_OrderedSceneList.empty()) {
        spdlog::info("Can't get next scene because scene list is empty.");
        return {};
    }

    if (!m_CurrentScene.has_value()) {
        return std::get<SceneHandle>(m_OrderedSceneList.at(0));
    }

    uint32_t i = 0;
    for (i = 0; i < m_OrderedSceneList.size(); ++i) {
        auto [handle, _] = m_OrderedSceneList.at(i);
        if (handle == m_CurrentScene.value())
            break;
    }
    if (i + 1 >= m_OrderedSceneList.size()) {
        spdlog::error("Already at end of ordered scene list.");
        return {};
    }

    return std::get<SceneHandle>(m_OrderedSceneList.at(i + 1));
}

void SceneManager::switchTo(SceneHandle scene, float delay) {
    if (!isSceneValid(scene)) {
        spdlog::error("Scene is invalid. Cannot switch to it.");
        return;
    }
    if (m_NextScene.has_value()) {
        spdlog::error("Cannot switch to another scene when another scene is in "
                      "transition.");
        return;
    }
    if (isSceneActive(scene)) {
        spdlog::error("Scene is already active. Cannot switch to it.");
        return;
    }

    m_SceneSwitchDelay = std::max(0.0f, delay);
    m_NextScene = scene;
}

std::vector<SceneHandle> SceneManager::getAllScenes() const {
    std::vector<SceneHandle> handles;
    for (auto &[handle, _] : m_OrderedSceneList) {
        handles.push_back(handle);
    }
    return handles;
}

SceneHandle SceneManager::findScene(std::string_view name) const {
    auto sceneIt = m_NameToHandle.find(std::string(name));
    if (sceneIt != m_NameToHandle.cend())
        return sceneIt->second;
    return {};
}

Scene *SceneManager::getSceneMut(SceneHandle handle) {
    if (!isSceneValid(handle)) {
        return nullptr;
    }
    return m_Scenes.at(handle.index).get();
}

const Scene *SceneManager::getScene(SceneHandle handle) const {
    if (!isSceneValid(handle)) {
        return nullptr;
    }
    return m_Scenes.at(handle.index).get();
}

void SceneManager::onUpdate(float dt) {
    bool inTransition = m_NextScene.has_value();
    if (m_SceneSwitchDelay > 0.0f && inTransition) {
        m_SceneSwitchDelay -= dt;
    }
    if (m_SceneSwitchDelay <= 0.0f && inTransition) {
        m_SceneSwitchDelay = 0.0f;
    }
}

void SceneManager::handleSwitch() {
    if (m_SceneSwitchDelay == 0.0f && m_NextScene.has_value()) {
        m_CurrentScene = m_NextScene;
        m_NextScene = std::nullopt;
    }
}

SceneHandle SceneManager::getActiveScene() const {
    return m_CurrentScene.value_or(SceneHandle{0, 0});
}

void SceneManager::onAttach() { spdlog::debug("Scene: Attached"); }

void SceneManager::onDettach() { spdlog::debug("Scene: Detached"); }

void SceneManager::onRender() {
    // for (auto &e : getEntities<MeshComp>()) {
    //     auto &modelComp = e.getComponent<MeshComp>();
    //     auto &tc = e.getComponent<TransformComp>();
    //
    //     // m_Renderer->drawMesh(modelComp.id, tc.worldMatrix);
    // }
}
} // namespace SYN
