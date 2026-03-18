#include "Buffer.h"
#include "Device.h"
#include "Image.h"
#include "StagingBuffer.h"
#include "Swapchain.h"
#include "renderer/IBackend.h"
#include <optional>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct VmaAllocator_T;

namespace SYN::VK {

struct FrameData {
    VkCommandPool graphicsCmdPool{};
    VkCommandBuffer graphicsCmdBuffer{};
    VkFence renderFinishedFence{};
    VkSemaphore imageAvailableSemaphore{};

    Image depthImage{};
};

class VulkanBackend : public IBackend {
  public:
    VulkanBackend() = default;
    ~VulkanBackend() = default;

    void init(Window &window) override;

    void render(Window &window) override;
    void shutdown() override;

  private:
    static constexpr uint32_t c_MaxFramesInFlight{2};
    static constexpr uint32_t c_TextureBinding{0};
    static constexpr uint32_t c_MaxBindlessTextures{1024};

    void initContext(Window &window);
    void initDescriptorSetLayout();
    void initPipelineLayout();
    void initDescriptorSets();
    void initFrameData(const Swapchain &swapchain);

    std::optional<uint32_t>
    beginFrame(Window &window); // returns currentImageIndex;
    void recordRenderCmd(uint32_t currentImageIndex);
    void endFrame(Window &window, uint32_t currentImageIndex);

    void recreateSwapchain(Window &window);

    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    Device m_Device{};

    VmaAllocator_T *m_Allocator{};

    Swapchain m_Swapchain{};
    VkCommandPool m_TransientCmdPool{};

    StagingBuffer m_StagingBuffer{};
    Buffer m_VertexBuffer{};

    VkPipelineLayout m_BindlessPipelineLayout{};

    VkDescriptorPool m_BindlessDescriptorPool{};
    VkDescriptorSetLayout m_BindlessDescriptorSetLayout{};
    VkDescriptorSet m_BindlessDescriptorSet{};

    VkPipeline m_GraphicsPipeline{};

    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    std::vector<Image> uploadedImages;

    uint32_t m_CurrentFrameIndex{};
};

} // namespace SYN::VK
