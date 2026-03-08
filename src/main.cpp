#include <PuzzleEngine/core/Application.h>

#include <spdlog/spdlog.h>

int main() {
    // todo move outside engine
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYN::Application>();
    app->init();
    app->run();
    app->shutdown();
}
