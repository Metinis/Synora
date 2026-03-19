#pragma once
#include "IBackend.h"

namespace SYN {
class Window;

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    void init(Window &window);
    void render(Window &window);
    void addMesh(UUID meshID, const MeshData& meshData);
    //could make a renderable object struct with meshID, materialID etc, all that are needed for drawing
    void drawMesh(UUID meshID);
    void shutdown();
    [[nodiscard]] IBackend* getBackend() const {return m_Backend.get();}

  private:
    std::unique_ptr<IBackend> m_Backend;
};

} // namespace SYN
