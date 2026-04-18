#include <SynoraEngine/core/Application.h>

int main() {
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYN::Application>();

    app->init();

    app->run();
    app->shutdown();
}
