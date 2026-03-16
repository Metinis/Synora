#include "Device.h"
#include "Swapchain.h"
#include "renderer/IBackend.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {

struct FrameData {
    VkCommandPool graphicsCmdPool{};
    VkCommandBuffer graphicsCmdBuffer{};
    VkFence renderFinishedFence{};
    VkSemaphore imageAvailableSemaphore{};
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

    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    Device m_Device{};
    Swapchain m_Swapchain{};
    VkPipelineLayout m_GraphicsPipelineLayout{};
    VkPipeline m_GraphicsPipeline{};
    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    uint32_t m_CurrentFrameIndex{};
};

} // namespace SYN::VK
