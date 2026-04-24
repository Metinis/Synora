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
#include "renderer/backends/IRenderDevice.h"
#include "renderer/backends/vulkan/GraphicsContext.h"
#include "renderer/backends/vulkan/Limits.h"

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

class VulkanRenderDevice : public IRenderDevice {
  public:
    VulkanRenderDevice() = default;
    ~VulkanRenderDevice() = default;

    void init(Window *window) override;

    BufferHandle createBuffer(const BufferDesc &desc) override;
    void destroyBuffer(BufferHandle handle) override;

    TextureHandle createTexture(const TextureDesc &desc) override;

    void destroyTexture(TextureHandle handle) override;

    AttachmentHandle createAttachment(const AttachmentDesc &desc) override;
    void destroyAttachment(AttachmentHandle &attachment) override;

    PipelineHandle createPipeline(const GraphicsPipelineDesc &desc) override;
    void destroyPipeline(PipelineHandle &pipeline) override;

    void recreateSwapchain(Window &window);

    IGraphicsContext *makeGraphicsContext() override;

    inline const Device &getDevice() const { return m_Device; }
    inline const VmaAllocator &getAllocator() const { return m_Allocator; }
    inline const Swapchain &getSwapchain() const { return m_Swapchain; }

    inline AttachmentHandle
    getSwapchainAttachmentHandle(uint32_t imageIndex) const {
        return m_SwapchainAttachmentHandles.at(imageIndex);
    }

    inline Buffer &getBuffer(BufferHandle handle) { return m_Buffers[handle]; }
    inline Texture &getTexture(TextureHandle handle) {
        return m_Textures[handle];
    }
    inline VkPipeline &getPipeline(PipelineHandle handle) {
        return m_Pipelines[handle];
    }
    inline Attachment &getAttachment(AttachmentHandle handle) {
        return m_Attachments[handle];
    }

    inline StagingBuffer &getStagingBuffer() { return m_StagingBuffer; }

    inline VkDescriptorSet getUBODescriptorSet() const {
        return m_UBODescriptorSet;
    }
    inline VkPipelineLayout getGraphicsPipelineLayout() const {
        return m_GraphicsPipelineLayout;
    }
    inline VkDescriptorSet getBindlessDescriptorSet() const {
        return m_BindlessDescriptorSet;
    }

    void shutdown() override;

  private:
    void initContext(Window *window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const Swapchain &swapchain);
    void initImGUI(Window *window);
    void initSamplers();

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

    std::array<VkSampler, Limits::c_SamplerCount> m_Samplers{};

    StagingBuffer m_StagingBuffer{};

    // imgui
    VkDescriptorPool m_ImGUIDescriptorPool{};

    // resources
    SlotMap<BufferHandle, Buffer> m_Buffers{};
    SlotMap<TextureHandle, Texture> m_Textures{};
    SlotMap<PipelineHandle, VkPipeline> m_Pipelines{};

    SlotMap<AttachmentHandle, Attachment> m_Attachments{};
    std::vector<AttachmentHandle> m_SwapchainAttachmentHandles{};

    SlotMap<ReceiptHandle, Receipt> m_Receipts{};

    std::vector<std::unique_ptr<VulkanGraphicsContext>> m_GraphicsContexts;
};

} // namespace SYN::VK
