#include <iostream>
#include <format>

#include <miniaudio.h>
#include <entt/entt.hpp>
#include <Jolt/Jolt.h>

int main() {
    std::string s = std::format("Hello, {}!", "world");
    std::cout << s << std::endl;
}
