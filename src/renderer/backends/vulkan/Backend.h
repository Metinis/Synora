#include "core/Buffer.h"
#include "core/Device.h"
#include "core/DynamicUBO.h"
#include "core/Image.h"
#include "core/SlotMap.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"
#include "renderer/IBackend.h"
#include "renderer/RenderTypes.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN::VK {

class VulkanBackend : public IBackend {
  public:
    VulkanBackend() = default;
    ~VulkanBackend() = default;

    void init(Window *window) override;

    BufferHandle createBuffer(const BufferDesc &desc) override;
    void uploadToBuffer(BufferHandle handle, size_t size, void *data) override;
    void destroyBuffer(BufferHandle &handle) override;

    TextureHandle createTexture(const TextureDesc &desc) override;
    void uploadToTexture(TextureHandle handle, uint32_t width, uint32_t height,
                         uint32_t stride, void *data) override;
    void destroyTexture(TextureHandle &handle) override;

    void beginFrame(Window &window) override;
    void endFrame(Window &window) override;

    RenderPassHandle beginRenderPassCmd(const RenderPassDesc &desc) override;
    void endRenderPassCmd(RenderPassHandle &renderPass) override;

    void drawCmd(BufferHandle vertexBuffer, size_t nVertices) override;

    TextureHandle getSwapchainTextureCmd() override;
    Viewport getSwapchainViewport() override;

    void shutdown() override;

  private:
    struct FrameData {
        VkCommandPool graphicsCmdPool{};
        VkCommandBuffer graphicsCmdBuffer{};

        VkFence renderFinishedFence{};
        VkSemaphore imageAvailableSemaphore{};

        DynamicUBO UBOBuffer;

        uint32_t swapchainImageIndex{};
    };

    struct PushConstants {
        VkDeviceAddress vertexBuffer;
    };

    static constexpr uint32_t c_MaxFramesInFlight{2};
    static constexpr uint32_t c_TextureBinding{0};
    static constexpr uint32_t c_MaxBindlessTextures{1024};

    void initContext(Window *window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const Swapchain &swapchain);

    void recordRenderCmd(uint32_t currentImageIndex);

    void recreateSwapchain(Window &window);

    uint32_t m_CurrentFrameIndex{};

    // context
    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    Device m_Device{};

    VmaAllocator_T *m_Allocator{};

    Swapchain m_Swapchain{};

    // global
    VkCommandPool m_TransientCmdPool{};

    VkPipelineLayout m_PipelineLayout{};

    VkDescriptorPool m_DescriptorPool{};

    VkDescriptorSetLayout m_BindlessDescriptorSetLayout{};
    VkDescriptorSetLayout m_UBODescriptorSetLayout{};

    VkDescriptorSet m_BindlessDescriptorSet{};
    VkDescriptorSet m_UBODescriptorSet{};

    std::vector<uint32_t> m_BindlessTextureIndexFreelist{};

    VkSampler m_DefaultSampler{};
    VkPipeline m_GraphicsPipeline{};

    StagingBuffer m_StagingBuffer{};

    TextureHandle m_SwapchainAttachmentHandle{};

    // per frame
    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    // resources
    SlotMap<BufferHandle, Buffer> m_Buffers{};
    SlotMap<TextureHandle, Image> m_Textures{};
    SlotMap<TextureHandle, std::array<Image, c_MaxFramesInFlight>>
        m_Attachments{};
};

} // namespace SYN::VK
