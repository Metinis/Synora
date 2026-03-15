#include "Device.h"
#include "Swapchain.h"
#include "renderer/IBackend.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {

struct VulkanState {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    Device device;
    Swapchain swapchain;
    VkPipelineLayout graphicsPipelineLayout;
    VkPipeline graphicsPipeline;
};

class VulkanBackend : public IBackend {
  public:
    VulkanBackend() = default;
    ~VulkanBackend() = default;

    void init(SYN::Window &window) override;
    void shutdown() override;

  private:
    VulkanState m_State;
};

} // namespace SYN::VK
