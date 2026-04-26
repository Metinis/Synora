#pragma once
#include <vulkan/vulkan.h>

namespace SYN::VK {
struct Device;

class BindlessSet {
  public:
    ~BindlessSet();

    void create(const Device &device);
    void destroy(const Device &device);

    inline VkDescriptorSet getSet() { return m_DescriptorSet; }
    inline VkDescriptorSetLayout getLayout() { return m_DescriptorSetLayout; }

  private:
    VkDescriptorPool m_DescriptorPool{VK_NULL_HANDLE};
    VkDescriptorSetLayout m_DescriptorSetLayout{VK_NULL_HANDLE};
    VkDescriptorSet m_DescriptorSet{VK_NULL_HANDLE};

    std::vector<uint32_t> m_BindlessTextureIndexFreelist{};
    std::vector<uint32_t> m_BindlessStorageImageFreelist{};
};
} // namespace SYN::VK
