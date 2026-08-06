#include <SynoraEngine/scene/RuntimeCompManager.h>
//#include <spdlog/spdlog.h>

using namespace SYN;
void RuntimeCompManager::init(EngineContext *ctx) {
    m_Ctx = ctx;
}
void RuntimeCompManager::registerComponent(const CompDesc &desc) {
    if (m_Components.contains(desc.name)) {
        //spdlog::warn("Component already registered");
    }
    //spdlog::trace("Component registered = {}", desc.name);
    m_Components[desc.name] = desc;
}
CompDesc RuntimeCompManager::getComponentDesc(const std::string &name) {
    return m_Components[name.c_str()];
}
void RuntimeCompManager::removeComponent(const char *name) {
    m_Components.erase(name);
}