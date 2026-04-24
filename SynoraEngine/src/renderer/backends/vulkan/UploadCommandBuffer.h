#pragma once
#include "SynoraEngine/renderer/RenderTypes.h"
#include <vulkan/vulkan.h>

namespace SYN {
class RenderDevice;

class UploadCommandBuffer {
  private:
    UploadCommandBuffer(RenderDevice *renderDevice);

  public:
    ~UploadCommandBuffer();

    void uploadToBuffer(BufferHandle handle, size_t size, const void *data);

    void uploadToTexture(TextureHandle handle, std::span<const void *> data,
                         uint32_t width, uint32_t height);
    inline void uploadToTexture(TextureHandle handle, const void *data,
                                uint32_t width, uint32_t height) {
        uploadToTexture(handle, std::span(&data, 1), width, height);
    }

  private:
    friend RenderDevice;

    void reset();

    RenderDevice *m_RenderDevice;
};

} // namespace SYN
