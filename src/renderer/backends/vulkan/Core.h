#include "Buffer.h"
#include "Device.h"
#include "Image.h"
#include "StagingBuffer.h"
#include "Swapchain.h"
#include "renderer/IBackend.h"
#include <map>
#include <optional>
#include <stb_image.h>
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

using VKResource = std::variant<VKMesh, VKMaterial>;

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

    void init(Window *window) override;

    void addMesh(UUID meshID, const MeshData &meshData) override;
    // could make a renderable object struct with meshID, materialID etc, all
    // that are needed for drawing
    void drawMesh(UUID meshID) override;

    void render(Window &window) override;

    void shutdown() override;

  private:
    struct LoadedImage {
        int width{};
        int height{};
        int channels{};
        stbi_uc *data{};
    };

    static constexpr uint32_t c_MaxFramesInFlight{2};
    static constexpr uint32_t c_TextureBinding{0};
    static constexpr uint32_t c_MaxBindlessTextures{1024};

    void initContext(Window *window);
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

    VkPipelineLayout m_BindlessPipelineLayout{};

    VkDescriptorPool m_BindlessDescriptorPool{};
    VkDescriptorSetLayout m_BindlessDescriptorSetLayout{};
    VkDescriptorSet m_BindlessDescriptorSet{};

    VkPipeline m_GraphicsPipeline{};

    std::array<FrameData, c_MaxFramesInFlight> m_FrameData{};
    std::vector<VkSemaphore> m_RenderFinishedSemaphores{};

    VkSampler m_DefaultSampler{};

    StagingBuffer m_StagingBuffer{};
    Buffer m_VertexBuffer{};
    std::vector<Image> m_Textures;

    uint32_t m_CurrentFrameIndex{};

    std::unordered_map<UUID, VKResource> m_Resources{};
};

} // namespace SYN::VK
