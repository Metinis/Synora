#include "efsw/efsw.hpp"
#include "SynoraEngine/scene/ScriptManager.h"
#include <SynoraEngine/core/Application.h>
#include <dlfcn.h>
#include <spdlog/spdlog.h>
#include <tinyfiledialogs.h>

#define SYN_LOG_SYSTEM_MANAGEMENT
#ifdef SYN_LOG_SYSTEM_MANAGEMENT
#define SYN_LOG_SYSTEM_M(...) spdlog::debug(__VA_ARGS__)
#else
#define SYN_LOG_SYSTEM_M(...)
#endif

using namespace SYN;

namespace fs = std::filesystem;

void ScriptManager::loadSystem(ISystem *system) {
  system->onLoad();
}
void ScriptManager::unloadSystem(ISystem *system) {
  system->onUnload();
}
void ScriptManager::init(EngineContext *ctx, const std::filesystem::path& path) {
  m_Ctx = ctx;

  std::string currentDir = std::filesystem::current_path().string();
  std::filesystem::path gamePath;
  if(path == "") {
    const char *folder = tinyfd_selectFolderDialog("Select Game Folder", currentDir.c_str());
    gamePath = std::filesystem::path(folder);
  } else {
    gamePath = currentDir;
  }

  if (!exists(gamePath / "systems")) {
    std::filesystem::create_directory(gamePath / "systems");
    spdlog::debug("Systems dir not found.. Creating a new one");
  }
  if (!exists(gamePath / "assets")) {
    std::filesystem::create_directory(gamePath / "assets");
    spdlog::debug("Assets dir not found.. Creating a new one");
  }
  initAllSystems(gamePath / "systems");
  std::filesystem::remove_all(gamePath / "systems" / ".hotreload");

  m_Listener.onFileChanged = [this](const std::string &gamePath, efsw::Action action) {
    handleFileChanged(gamePath, action);
  };


}
void ScriptManager::onAttach() {

}
static void dllUnloadSystem(System& system) {
  system.system.reset();

  if (system.handle) {
    dlclose(system.handle);
    system.handle = nullptr;
  }
}
static System dllLoadSystem(const std::filesystem::path &path, EngineContext *ctx) {
  SYN_LOG_SYSTEM_M("Loaded system {}", path.string());
  void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);

  if (!handle) {
    spdlog::error("Failed to load {}: {}", path.c_str(), dlerror());
    //return {};
  }

  auto create = (ISystem*(*)())dlsym(handle, "createSystem");
  auto destroy = (void(*)(ISystem*))dlsym(handle, "destroySystem");
  auto registerComps = (void(*)(RuntimeCompManager*))dlsym(handle, "registerComponents");

  std::unique_ptr<ISystem, void(*)(ISystem*)> system(create(), destroy);
  //todo optionally load a system, allow for just comps
  if (registerComps) {
    registerComps(ctx->compManager.get());
  }

  system->init(ctx);

  System sys {
    .handle = handle,
      .system = std::move(system),
  };

  return sys;
}
void ScriptManager::reloadSystem(const std::string& path) {
  auto& system = m_Systems.at(path);
  dllUnloadSystem(system);

  //use a new path to avoid caching and windows issues
  static uint64_t reloadCounter = 0;

  const auto& original = path;
  std::filesystem::path fsPath(path);
  auto cacheDir = fsPath.parent_path() / ".hotreload";
  std::filesystem::create_directories(cacheDir);

  auto loaded = cacheDir /
    (fsPath.stem().string() +
     "_reload_" +
     std::to_string(reloadCounter++) +
     fsPath.extension().string());

  std::filesystem::copy_file(
      original,
      loaded,
      std::filesystem::copy_options::overwrite_existing);

  auto sys = dllLoadSystem(loaded.c_str(), m_Ctx);
  //should reload?
  //loadSystem(sys.system.get());
  //use the original path
  m_Systems.insert_or_assign(path, std::move(sys));
}
void ScriptManager::onUpdate(float dt) {
  if(m_Ctx->isGameRunning) {
    for(auto it = m_PendingReloads.begin(); it != m_PendingReloads.end();) {
      it->timer -= dt;

      if(it->timer <= 0.0f) {
        reloadSystem(it->path);
        it = m_PendingReloads.erase(it);
      }
      else {
        ++it;
      }
    }
    for (auto& system : m_Systems) {
      system.second.system->onUpdate(dt);
    }
  }
}
void ScriptManager::onDetach() {
  unloadAllSystems();
}
void ScriptManager::loadAllSystems() {
  for (auto& system : m_Systems) {
    loadSystem(system.second.system.get());
  }
}
void ScriptManager::unloadAllSystems() {
  for (auto& system : m_Systems) {
    unloadSystem(system.second.system.get());
  }
}

void ScriptManager::handleFileChanged(const std::string &path, efsw::Action action) {
  bool isDLL = path.ends_with(".so") || path.ends_with(".dll") || path.ends_with(".dylib");
  bool isSource = path.ends_with(".cpp");

  if ((!isSource && !isDLL) || path.starts_with(".#")) {
    return;
  }
  SYN_LOG_SYSTEM_M("File action {} in {}", path, efsw::actionToString(action));
  if (action == efsw::Action::Modified && isDLL) {
    //reload the system if dll is changed
    if (!m_Systems.contains(path)) {
      spdlog::error("Path not mapped! {}", path.c_str());
      return;
    }
    m_PendingReloads.push_back({.path = path, .timer = 0.25f});
    return;

  }
  if (action == efsw::Action::Modified && isSource) {
    //build dll if file is changed
    std::string target = std::filesystem::path(path).stem().string();

    std::string cmd = "cmake --build " + std::filesystem::current_path().parent_path().string() + " --config Debug --target " + target;

    int result = std::system(cmd.c_str());

    if (result != 0) {
      spdlog::error("Failed to build {}", target);
    }
  }
}

// returns success/failure
bool ScriptManager::initAllSystems(const std::filesystem::path &path) {
  //load dlls from path if it exists
  efsw::WatchID watchID = m_Ctx->fileWatcher->addWatch( path.c_str(), &m_Listener, true );

  for (const auto& entry : fs::directory_iterator(path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".so") {
      auto sys = dllLoadSystem(entry.path().string(), m_Ctx);
      m_Systems.emplace(entry.path().string(), std::move(sys));
      spdlog::info("Loaded DLL {}", entry.path().c_str());
    }
  }
  return true;
}
