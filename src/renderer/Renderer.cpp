#include "Renderer.h"
#include "backends/vulkan/Core.h"

using namespace SYN;

void SYN::Renderer::init(EngineContext* ctx) {
    m_Backend = std::make_unique<VK::VulkanBackend>();
    m_Backend->init(ctx->window.get());
}

void SYN::Renderer::render(Window &window) { m_Backend->render(window); }

void Renderer::addMesh(UUID meshID, const MeshData &meshData) {
    m_Backend->addMesh(meshID, meshData);
}

void Renderer::drawMesh(UUID meshID) {
    m_Backend->drawMesh(meshID);
}

void SYN::Renderer::shutdown() { m_Backend->shutdown(); }
