#include "Renderer.h"
#include "PuzzleEngine/core/Window.h"
#include "backends/vulkan/Backend.h"
#include "renderer/IBackend.h"
#include "renderer/RenderTypes.h"

namespace {
struct PushConstants {
    uint64_t vertexBuffer;
    uint32_t textureIndex;
};
} // namespace

using namespace SYN;

void SYN::Renderer::init(EngineContext *ctx) {
    m_Backend = std::make_unique<VK::VulkanBackend>();
    m_Backend->init(ctx->window.get());
    m_Window = ctx->window.get();
}

void SYN::Renderer::render(Window &window) {}

void Renderer::addMesh(UUID meshID, const MeshData &meshData) {

    std::vector<Vertex> vertices{
        Vertex{
            .pos = {-0.5, -0.5, 1.0},
            .u = 0,
            .v = 0,
        },
        {
            .pos = {0.0, -0.5, 1.0},
            .u = 1,
            .v = 0,
        },
        {
            .pos = {-0.5, 0.5, 1.0},
            .u = 0,
            .v = 1,
        },

        {
            .pos = {0.0, -0.5, 1.0},
            .u = 1,
            .v = 0,
        },
        {
            .pos = {0.0, 0.5, 1.0},
            .u = 1,
            .v = 1,
        },
        {
            .pos = {-0.5, 0.5, 1.0},
            .u = 0,
            .v = 1,
        },
    };

    BufferHandle vertexBuffer{m_Backend->createBuffer(
        BufferDesc{.size = vertices.size() * sizeof(Vertex)})};
    m_Backend->uploadToBuffer(vertexBuffer, vertices.size() * sizeof(Vertex),
                              vertices.data());

    m_Buffers[meshID] = vertexBuffer;

    stbi_set_flip_vertically_on_load(true);

    int imageWidth{};
    int imageHeight{};
    int channelsInImage{};
    std::string imagePath{"resources/textures/missing_texture.png"};
    stbi_uc *imageBytes{stbi_load(imagePath.c_str(), &imageWidth, &imageHeight,
                                  &channelsInImage, 4)};

    assert(channelsInImage == 4);
    if (imageBytes == nullptr) {
        spdlog::warn("Could not load {}", imagePath);
        return;
    }

    TextureHandle texture{m_Backend->createTexture(
        TextureDesc{.width = static_cast<uint32_t>(imageWidth),
                    .height = static_cast<uint32_t>(imageHeight),
                    .type = TextureType::srgb})};
    Viewport swapchainViewport{m_Backend->getSwapchainViewport()};

    m_Backend->uploadToTexture(texture, imageWidth, imageHeight,
                               channelsInImage, imageBytes);
    stbi_image_free(imageBytes);

    m_Textures[meshID] = texture;

    AttachmentHandle depthAttachment{m_Backend->createAttachment(
        AttachmentDesc{.type = TextureType::depth})};
    m_DepthAttachment = depthAttachment;
}

void Renderer::drawMesh(UUID meshID) {
    m_Backend->beginFrame(*m_Window);
    AttachmentHandle swapchainAttachment{
        m_Backend->getSwapchainAttachmentCmd()};
    std::array<WriteAttachment, 1> colorAttachments{
        WriteAttachment{
            .handle = swapchainAttachment,
            .clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f),
        },
    };
    WriteAttachment depthAttachment{.handle = m_DepthAttachment,
                                    .clearDepth = 0.f};

    Viewport swapchainViewport{m_Backend->getSwapchainViewport()};

    RenderPassDesc firstRenderPassDesc{.colorAttachments = colorAttachments,
                                       .depthAttachment = depthAttachment,
                                       .viewport = swapchainViewport};
    PushConstants pushConstants{
        .vertexBuffer = m_Backend->getBufferAddressCmd(m_Buffers[meshID]),
        .textureIndex = m_Backend->getShaderSamplerIndexCmd(m_Textures[meshID]),
    };

    m_Backend->setPushConstantsCmd(pushConstants);
    m_Backend->beginRenderPassCmd(firstRenderPassDesc);
    m_Backend->drawCmd(m_Buffers[meshID], 6);
    m_Backend->endRenderPassCmd();

    m_Backend->endFrame(*m_Window);
}

void SYN::Renderer::shutdown() {
    for (auto &[assetID, texture] : m_Textures) {
        m_Backend->destroyTexture(texture);
    }
    for (auto &[assetID, buffer] : m_Buffers) {
        m_Backend->destroyBuffer(buffer);
    }
    m_Backend->destroyAttachment(m_DepthAttachment);

    m_Backend->shutdown();
}
