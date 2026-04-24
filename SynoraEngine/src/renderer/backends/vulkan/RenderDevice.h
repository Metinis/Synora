#pragma once
#include "Limits.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "core/Buffer.h"
#include "core/Device.h"
#include "core/DynamicUBO.h"
#include "core/Image.h"
#include "core/SlotMap.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"
#include "renderer/backends/vulkan/GraphicsCommandBuffer.h"
#include "renderer/backends/vulkan/Limits.h"
#include "renderer/backends/vulkan/UploadCommandBuffer.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN::VK {

struct Texture {
    Image image;
    uint32_t bindlessSamplerIndex;
};

struct Attachment {
    Image image;
    AttachmentSize size;
    uint32_t bindlessSamplerIndex;
};

struct Receipt {
    VkSemaphore semaphore;
    uint64_t waitValue;
};

struct FrameData {
    VkCommandPool graphicsCmdPool{};
    VkCommandBuffer graphicsCmdBuffer{};

    VkFence renderFinishedFence{};
    VkSemaphore imageAvailableSemaphore{};

    DynamicUBO UBOBuffer;
};
} // namespace SYN::VK

namespace SYN {

class RenderDevice {
  public:
    RenderDevice() = default;
    ~RenderDevice() = default;

    void init(Window &window);

    BufferHandle createBuffer(const BufferDesc &desc);
    void destroyBuffer(BufferHandle handle);

    TextureHandle createTexture(const TextureDesc &desc);
    void destroyTexture(TextureHandle handle);

    AttachmentHandle createAttachment(const AttachmentDesc &desc);
    void destroyAttachment(AttachmentHandle &attachment);

    PipelineHandle createPipeline(const GraphicsPipelineDesc &desc);
    void destroyPipeline(PipelineHandle &pipeline);

    GraphicsCommandBuffer acquireGraphicsCmdBuffer();
    UploadCommandBuffer acquireUploadCmdBuffer();

    AttachmentHandle acquireSwapchainAttachment();

    bool beginFrame(Window &window);

    // ends cmd buffer recording
    void submitWork(GraphicsCommandBuffer &cmdBuffer);
    void submitWork(UploadCommandBuffer &cmdBuffer);
    // ends frame
    void present(Window &window);

    void shutdown();

  private:
    friend class GraphicsCommandBuffer;
    friend class UploadCommandBuffer;

    void initContext(Window *window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const VK::Swapchain &swapchain);
    void initImGUI(Window *window);
    void initSamplers();

    void recreateSwapchain(Window &window);

    inline AttachmentHandle
    getSwapchainAttachmentHandle(uint32_t imageIndex) const {
        return m_SwapchainAttachmentHandles.at(imageIndex);
    }

    inline VK::FrameData &getCurrentFrame() {
        return m_FrameData[m_CurrentFrameIndex];
    }

    // context
    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    VK::Device m_Device{};

    VmaAllocator_T *m_Allocator{};

    VK::Swapchain m_Swapchain{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};
    uint32_t m_CurrentSwapchainImageIndex{};

    // global
    VkCommandPool m_TransientCmdPool{};

    VkDescriptorPool m_DescriptorPool{};

    VkDescriptorSetLayout m_BindlessDescriptorSetLayout{};
    VkDescriptorSetLayout m_UBODescriptorSetLayout{};

    VkPipelineLayout m_GraphicsPipelineLayout;

    VkDescriptorSet m_BindlessDescriptorSet{};
    VkDescriptorSet m_UBODescriptorSet{};

    std::vector<uint32_t> m_BindlessTextureIndexFreelist{};

    std::array<VkSampler, VK::Limits::c_SamplerCount> m_Samplers{};

    VK::StagingBuffer m_StagingBuffer{};

    // per frame
    std::array<VK::FrameData, VK::Limits::c_MaxFramesInFlight> m_FrameData{};
    uint32_t m_CurrentFrameIndex{};

    // imgui
    VkDescriptorPool m_ImGUIDescriptorPool{};

    // resources
    VK::SlotMap<BufferHandle, VK::Buffer> m_Buffers{};
    VK::SlotMap<TextureHandle, VK::Texture> m_Textures{};
    VK::SlotMap<PipelineHandle, VkPipeline> m_Pipelines{};

    VK::SlotMap<AttachmentHandle, VK::Attachment> m_Attachments{};
    std::vector<AttachmentHandle> m_SwapchainAttachmentHandles{};

    VK::SlotMap<ReceiptHandle, VK::Receipt> m_Receipts{};
};

} // namespace SYN
