#pragma once
#include "PuzzleEngine/project/UUID.h"

struct MeshData;

namespace SYN {
class Window;

class IBackend {
  public:
    IBackend() = default;
    virtual ~IBackend() = default;

    virtual void init(Window &window) = 0;
    virtual void addMesh(UUID meshID, const MeshData& meshData) = 0;
    //could make a renderable object struct with meshID, materialID etc, all that are needed for drawing
    virtual void drawMesh(UUID meshID) = 0;
    virtual void render(Window &window) = 0;
    virtual void shutdown() = 0;
  private:
};

} // namespace SYN
