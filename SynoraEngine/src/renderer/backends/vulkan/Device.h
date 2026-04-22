#pragma once

#include "core/Buffer.h"
#include "core/Device.h"
#include "core/DynamicUBO.h"
#include "core/Image.h"
#include "core/SlotMap.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"
#include "renderer/RenderTypes.h"
#include "renderer/backends/IDevice.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN::VK {

struct FrameData {
    VkCommandPool graphicsCmdPool{};
    VkCommandBuffer graphicsCmdBuffer{};

    VkFence renderFinishedFence{};
    VkSemaphore imageAvailableSemaphore{};

    DynamicUBO UBOBuffer;

    std::vector<Image> imagesToFree;
    std::vector<Buffer> buffersToFree;
    std::vector<uint32_t> bindlessTexturesToFree;
    std::vector<uint32_t> bindlessCubeMapsToFree;
    std::vector<VkPipeline> pipelinesToFree;

    AttachmentHandle swapchainHandle{};
    uint32_t swapchainImageIndex{};
};

struct Texture {
    Image image;
    uint32_t bindlessSamplerIndex;
};

struct Attachment {
    Image image;
    AttachmentSize size;
    uint32_t bindlessSamplerIndex;
};

class VulkanDevice : public IDevice {
  public:
    VulkanDevice() = default;
    ~VulkanDevice() = default;

    void init(Window *window) override;

    BufferHandle createBuffer(const BufferDesc &desc) override;
    void destroyBuffer(BufferHandle handle) override;

    TextureHandle createTexture(const TextureDesc &desc) override;

    void destroyTexture(TextureHandle handle) override;

    AttachmentHandle createAttachment(const AttachmentDesc &desc) override;
    void destroyAttachment(AttachmentHandle &attachment) override;

    PipelineHandle createPipeline(const GraphicsPipelineDesc &desc) override;
    void destroyPipeline(PipelineHandle &pipeline) override;

    std::unique_ptr<IGraphicsContext> makeGraphicsContext() override;

    void shutdown() override;

  private:
    friend class VulkanGraphicsContext;

    static constexpr uint32_t c_MaxFramesInFlight{2};
    static constexpr uint32_t c_TextureBinding{0};
    static constexpr uint32_t c_CubeMapBinding{1};
    static constexpr uint32_t c_MaxBindlessTextures{1024};
    static constexpr uint32_t c_MaxBindlessCubeMaps{1024};
    static constexpr uint32_t c_MinGuarenteedPushConstantSize{128};

    void initContext(Window *window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const Swapchain &swapchain);
    void initImGUI(Window *window);

    void recreateSwapchain(Window &window);

    // context
    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    Device m_Device{};

    VmaAllocator_T *m_Allocator{};

    Swapchain m_Swapchain{};

    // global
    VkCommandPool m_TransientCmdPool{};

    VkDescriptorPool m_DescriptorPool{};

    VkDescriptorSetLayout m_BindlessDescriptorSetLayout{};
    VkDescriptorSetLayout m_UBODescriptorSetLayout{};

    VkPipelineLayout m_GraphicsPipelineLayout;

    VkDescriptorSet m_BindlessDescriptorSet{};
    VkDescriptorSet m_UBODescriptorSet{};

    std::vector<uint32_t> m_BindlessTextureIndexFreelist{};
    std::vector<uint32_t> m_BindlessCubeMapFreeList{};

    VkSampler m_DefaultSampler{};

    StagingBuffer m_StagingBuffer{};

    // imgui
    VkDescriptorPool m_ImGUIDescriptorPool{};

    // per frame
    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    // resources
    SlotMap<BufferHandle, Buffer> m_Buffers{};
    SlotMap<TextureHandle, Texture> m_Textures{};
    SlotMap<PipelineHandle, VkPipeline> m_Pipelines{};

    SlotMap<AttachmentHandle, Attachment> m_Attachments{};
};

} // namespace SYN::VK
