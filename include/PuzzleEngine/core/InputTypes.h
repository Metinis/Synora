#pragma once

namespace SYN {
using ActionID = uint32_t;
using Keycode = uint32_t;

// Least significant bit: is active?
// Next 7 bits: generation
// Last 8 bits: index
// We really shouldn't have anywhere near 256 active input contexts +
// we also should't be constantly removing input contexts to warrant
// over 128 generations.
struct InputContextHandle {
    uint16_t m_Data;
};

constexpr uint8_t MAX_INPUT_CONTEXTS = 255;
constexpr uint8_t MAX_INPUT_CONTEXT_GENERATIONS = 127;

enum class InputState : uint8_t { Down, Up };

enum class InputKey : Keycode {
    Up,
    Down,
    Left,
    Right,
    Space,
    Enter,
    Escape,
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    Tab,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12
};

// Map OS events into RawInput.
// Add more state as needed.
struct RawInput {
    InputState m_State;
    InputKey m_Code;
};

struct ActionBinding {
    InputState m_State;
    ActionID m_Action;
};

// Needs to be updated everytime a new input key is added.
constexpr uint32_t NUM_INPUT_KEYS = (uint32_t)InputKey::F12 + 1;

} // namespace SYN
