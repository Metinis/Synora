#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/scene/ScriptManager.h"

int main() {
  SYN::Application app{};
  app.init(PROJECT_ROOT);
  app.getCtx()->isGameRunning = true;
  app.getCtx()->scriptManager->loadAllSystems();
  app.run();
  app.shutdown();
}
