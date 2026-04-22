#include <SynoraEngine/core/Application.h>
#include "EditorApp.h"

int main() {
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYE::EditorApp>();

    app->init();

    app->run();
    app->shutdown();
}
