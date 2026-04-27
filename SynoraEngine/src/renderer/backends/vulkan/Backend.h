#include "../../../../include/SynoraEngine/renderer/RenderTypes.h"
#include "../../../../include/SynoraEngine/renderer/backends/IBackend.h"
#include "core/Buffer.h"
#include "core/Device.h"
#include "core/DynamicUBO.h"
#include "core/Image.h"
#include "core/SlotMap.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"

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
    void uploadToBuffer(BufferHandle handle, size_t size,
                        const void *data) override;
    void destroyBuffer(BufferHandle handle) override;

    TextureHandle createTexture(const TextureDesc &desc) override;
    void uploadToTexture(TextureHandle handle, std::span<const void *> data,
                         uint32_t width, uint32_t height) override;

    void destroyTexture(TextureHandle handle) override;

    AttachmentHandle createAttachment(const AttachmentDesc &desc) override;
    void destroyAttachment(AttachmentHandle &handle) override;

    PipelineHandle createPipeline(const GraphicsPipelineDesc &desc) override;
    void destroyPipeline(PipelineHandle &pipeline) override;

    void beginFrame(Window &window) override;
    void endFrame(Window &window) override;

    void beginRenderPassCmd(const RenderPassDesc &desc,
                            PipelineHandle pipeline) override;
    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const void *uniformData,
                            size_t uniformSize) override;
    void endRenderPassCmd() override;

    void setPushConstantsCmd(const void *data, size_t size) override;

    void drawCmd(size_t nVertices) override;
    void drawIndexedCmd(size_t nIndices) override;
    void drawImGUI() override;

    AttachmentHandle getSwapchainAttachmentCmd() override;

    uint32_t getShaderSamplerIndexCmd(TextureHandle texture) override;
    uint32_t getShaderSamplerIndexCmd(AttachmentHandle attachment) override;

    uint64_t getBufferAddressCmd(BufferHandle buffer) override;

    void shutdown() override;

  private:
    static constexpr uint32_t c_MaxFramesInFlight{2};
    static constexpr uint32_t c_TextureBinding{0};
    static constexpr uint32_t c_CubeMapBinding{1};
    static constexpr uint32_t c_MaxBindlessTextures{1024};
    static constexpr uint32_t c_MaxBindlessCubeMaps{1024};
    static constexpr uint32_t c_MinGuarenteedPushConstantSize{128};

    struct FrameData {
        VkCommandPool graphicsCmdPool{};
        VkCommandBuffer graphicsCmdBuffer{};

        VkFence renderFinishedFence{};
        VkSemaphore imageAvailableSemaphore{};

        DynamicUBO UBOBuffer;

        std::vector<Image> imagesToFree;
        std::vector<Buffer> buffersToFree;
        std::vector<uint32_t> bindlessTexturesToFree;
        std::vector<VkPipeline> pipelinesToFree;

        uint32_t swapchainImageIndex{};
    };

    struct Texture {
        Image image;
        uint32_t bindlessSamplerIndex;
    };
    struct Attachment {
        std::array<Image, c_MaxFramesInFlight> images;
        std::array<uint32_t, c_MaxFramesInFlight> bindlessSamplerIndices;
        AttachmentSize size;
        bool isSampleable;
    };

    void initContext(Window *window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const Swapchain &swapchain);
    void initImGUI(Window *window);

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

    AttachmentHandle m_SwapchainAttachmentHandle{};

    // imgui
    VkDescriptorPool m_ImGUIDescriptorPool{};

    // per frame
    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    // resources
    SlotMap<BufferHandle, Buffer> m_Buffers{};
    SlotMap<TextureHandle, Texture> m_Textures{};
    SlotMap<AttachmentHandle, Attachment> m_Attachments{};
    SlotMap<PipelineHandle, VkPipeline> m_Pipelines{};
};

} // namespace SYN::VK
