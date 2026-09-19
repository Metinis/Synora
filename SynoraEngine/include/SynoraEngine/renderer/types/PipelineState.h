#pragma once

namespace SYN {

struct PipelineState {
    enum class Cull { None, Back, Front };
    enum class FrontFace { Clockwise, CounterClockwise };
    enum class PolygonMode { Fill, Line, Point };

    enum class CompareFunc {
        Always,
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual
    };

    struct Depth {
        bool testEnabled = true;
        bool writeEnabled = true;
        CompareFunc test = CompareFunc::Less;
    };

    // This definition is incomplete since I don't know anything
    // about blending yet.
    struct Blend {
        bool enabled;
    };

    struct Stencil {
        bool testEnabled = false;

        uint8_t readMask = 0xFF, writeMask = 0xFF;
        uint8_t reference = 0xFF;

        struct Op {
            enum class Type {
                Keep,
                Zero,
                Replace,
                Increment,
                IncrementWrap,
                Decrement,
                DecrementWrap,
                Invert
            };
            Type stencilFail = Type::Keep;
            Type depthFail = Type::Keep;
            Type pass = Type::Keep;
            CompareFunc test = CompareFunc::Equal;
        };

        Op frontFace;
        Op backFace;
    };

    Depth depth;

    Cull cull = Cull::Back;
    FrontFace face = FrontFace::CounterClockwise;
    float lineWidth = 1.0f;

    PolygonMode polygonMode = PolygonMode::Fill;

    Stencil stencil;

    Blend blend;
};
}; // namespace SYN
