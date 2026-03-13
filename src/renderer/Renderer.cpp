#include "Renderer.h"
#include "backends/vulkan/Backend.h"

using namespace SYN;

void SYN::Renderer::init(Window &window) {
    m_Backend = std::make_unique<VulkanBackend>();
    m_Backend->init(window);
}
void SYN::Renderer::shutdown() { m_Backend->shutdown(); }
