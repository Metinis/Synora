#pragma once
#include "../Limits.h"
#include "../command_buffers/ComputeCommandBuffer.h"
#include "../command_buffers/GraphicsCommandBuffer.h"
#include "../command_buffers/UploadCommandBuffer.h"
#include "../core/Buffer.h"
#include "../core/Device.h"
#include "../core/DynamicUBO.h"
#include "../core/Image.h"
#include "../core/SlotMap.h"
#include "../core/StagingBuffer.h"
#include "../core/Swapchain.h"
#include "SynoraEngine/renderer/RenderTypes.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN {
struct Receipt {
    uint64_t waitValue;
};
}; // namespace SYN

namespace SYN::VK {

struct Texture {
    Image image;
    uint32_t bindlessSamplerIndex;
};

struct Attachment {
    Image image;
    AttachmentSize size;
    uint32_t bindlessTextureIndex;
    std::optional<uint32_t> bindlessStorageImageIndex;
};

struct FrameData {
    VkCommandPool graphicsCmdPool{};
    VkCommandBuffer graphicsCmdBuffer{};

    VkFence renderFinishedFence{};
    VkSemaphore imageAvailableSemaphore{};

    DynamicUBO UBOBuffer;
};

struct PendingSubmission {
    VkCommandBuffer cmdBuffer;
    uint64_t waitValue;
};

} // namespace SYN::VK

namespace SYN {

class RenderDevice {
  public:
    RenderDevice() = default;
    ~RenderDevice() = default;

    void init(Window &window);

    TextureFormat getSwapchainFormat() const;
    TextureType getSwapchainType() const;

    BufferHandle createBuffer(const BufferDesc &desc);
    void destroyBuffer(BufferHandle &handle);

    TextureHandle createTexture(const TextureDesc &desc);
    void destroyTexture(TextureHandle &handle);

    AttachmentHandle createAttachment(const AttachmentDesc &desc);
    void destroyAttachment(AttachmentHandle &attachment);

    PipelineHandle createPipeline(const GraphicsPipelineDesc &desc);
    PipelineHandle createPipeline(const ComputePipelineDesc &desc);
    void destroyPipeline(PipelineHandle &pipeline);

    // if true, receipt resources are good to be cleaned up, if false, theyre
    // still in use
    bool getReceiptStatus(Receipt receipt);

    GraphicsCommandBuffer acquireGraphicsCmdBuffer();
    UploadCommandBuffer acquireUploadCmdBuffer();
    ComputeCommandBuffer acquireComputeCmdBuffer();

    AttachmentHandle acquireSwapchainAttachment();

    bool beginFrame(Window &window);

    // ends cmd buffer recording
    Receipt submitWork(GraphicsCommandBuffer &cmdBuffer,
                       std::span<Receipt> waitReceipts);
    Receipt submitWork(UploadCommandBuffer &cmdBuffer,
                       std::span<Receipt> waitReceipts);
    Receipt submitWork(ComputeCommandBuffer &cmdBuffer,
                       std::span<Receipt> waitReceipts);

    inline Receipt submitWork(GraphicsCommandBuffer &cmdBuffer) {
        return submitWork(cmdBuffer, {});
    }
    inline Receipt submitWork(UploadCommandBuffer &cmdBuffer) {
        return submitWork(cmdBuffer, {});
    }
    inline Receipt submitWork(ComputeCommandBuffer &cmdBuffer) {
        return submitWork(cmdBuffer, {});
    }

    // ends frame
    void present(Window &window);

    void shutdown();

  private:
    friend class GraphicsCommandBuffer;
    friend class UploadCommandBuffer;
    friend class ComputeCommandBuffer;

    void initContext(Window *window);

    void initUBOSetLayout();
    void initBindlessSetLayout(uint32_t textureDescriptorCount,
                               uint32_t samplerDescriptorCount,
                               uint32_t storageImageDescriptorCount);
    void initDescriptorPool(uint32_t textureDescriptorCount,
                            uint32_t samplerDescriptorCount,
                            uint32_t storageImageDescriptorCount);
    void initPipelineLayout();

    void initFrameData(const VK::Swapchain &swapchain);
    void initImGUI(Window *window);
    void initSamplers();

    void recreateSwapchain(Window &window);

    void pollSubmissions();

    VkCommandBuffer acquireCommandBuffer();

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

    VkDescriptorSetLayout m_BindlessSetLayout{};
    VkDescriptorSetLayout m_UBOSetLayout{};

    VkPipelineLayout m_PipelineLayout{};

    VkDescriptorSet m_BindlessSet{};
    VkDescriptorSet m_UBOSet{};

    std::vector<uint32_t> m_BindlessTextureIndexFreelist{};
    std::vector<uint32_t> m_BindlessStorageImageFreelist{};

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

    // these are different cus you may want signal at different pipeline stages
    // for optimization
    VkSemaphore m_GPUSubmissionSemaphore{}; // for gpu-gpu sync
    VkSemaphore m_CPUSubmissionSemaphore{}; // for cpu-gpu sync

    uint64_t m_SubmissionCount{};
    std::queue<VK::PendingSubmission> m_PendingSubmissions{};

    std::vector<VkCommandBuffer> m_FreeCmdBuffers{};
    std::vector<VkFence> m_FreeFences{};
};

} // namespace SYN
