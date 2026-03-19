#include "Buffer.h"
#include "Device.h"
#include "StagingBuffer.h"
#include "Swapchain.h"
#include "renderer/IBackend.h"
#include <optional>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "PuzzleEngine/project/UUID.h"

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN::VK {

struct VKMesh {
    int temp;
};

struct VKMaterial {
    int temp;
};

using VKResource = std::variant<
    VKMesh,
    VKMaterial
>;

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

    void addMesh(UUID meshID, const MeshData& meshData) override;
    //could make a renderable object struct with meshID, materialID etc, all that are needed for drawing
    void drawMesh(UUID meshID) override;

    void render(Window &window) override;

    void shutdown() override;

  private:
    void initContext(Window &window);
    void initPipeline();
    void initFrameData();

    std::optional<uint32_t>
    beginFrame(Window &window); // returns currentImageIndex;
    void recordRenderCmd(uint32_t currentImageIndex);
    void endFrame(Window &window, uint32_t currentImageIndex);

    void recreateSwapchain(Window &window);

    static constexpr uint32_t c_MaxFramesInFlight{2};

    VkInstance m_Instance{};
    VkSurfaceKHR m_Surface{};
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger{};
    Device m_Device{};

    VmaAllocator_T *m_Allocator{};

    Swapchain m_Swapchain{};

    StagingBuffer m_StagingBuffer{};
    Buffer m_VertexBuffer{};

    VkPipelineLayout m_GraphicsPipelineLayout{};

    VkPipeline m_GraphicsPipeline{};

    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    uint32_t m_CurrentFrameIndex{};

    std::unordered_map<UUID, VKResource> m_Resources{};
};

} // namespace SYN::VK
