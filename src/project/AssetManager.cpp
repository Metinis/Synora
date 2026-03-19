#include "PuzzleEngine/project/AssetManager.h"

void SYN::AssetManager::init(EngineContext* ctx) {
    m_Renderer = ctx->renderer.get();
}
