#include <iostream>
#include <format>

int main() {
    std::string s = std::format("Hello, {}!", "world");
    std::cout << s << std::endl;
}
