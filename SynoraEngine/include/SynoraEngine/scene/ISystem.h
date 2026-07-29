#pragma once

namespace SYN {
struct EngineContext;

class ISystem {
public:
    virtual void init(EngineContext* ctx) = 0;
    virtual void onLoad() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onUnload() = 0;
    virtual ~ISystem() = default;
protected:
    EngineContext* m_Ctx{};
};
}