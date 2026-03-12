#include "Renderer.h"
#include "backends/vulkan/Backend.h"

using namespace SYN;

void Renderer::init() {
    m_Backend = std::make_unique<VulkanBackend>();
    m_Backend->init();
}
void Renderer::shutdown() { m_Backend->shutdown(); }
