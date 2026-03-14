#include "Device.h"
#include "renderer/IBackend.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN {

struct VulkanState {
    VkInstance instance;
    VkSurfaceKHR surface;
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    Device device;
};

class VulkanBackend : public IBackend {
  public:
    VulkanBackend() = default;
    ~VulkanBackend() = default;

    void init(Window &window) override;
    void shutdown() override;

  private:
    VulkanState m_State;
};

} // namespace SYN
