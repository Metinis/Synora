#include "renderer/IBackend.h"

namespace SYN {

class VulkanBackend : public IBackend {
  public:
    VulkanBackend() = default;
    ~VulkanBackend() = default;

    void init() override;
    void shutdown() override;

  private:
};

} // namespace SYN
