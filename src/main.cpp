#include "core/Application.h"

int main() {
    //todo move outside engine
    auto* app = new SYN::Application();
    app->init();
    app->run();
    app->shutdown();
}
