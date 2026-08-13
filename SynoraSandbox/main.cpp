#include "SynoraEngine/core/Application.h"

int main() {
  SYN::Application app{};
  app.init(PROJECT_ROOT);
  app.getCtx()->isGameRunning = true;
  app.run();
  app.shutdown();
}
