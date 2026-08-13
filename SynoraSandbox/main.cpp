#include "SynoraEngine/core/Application.h"

int main() {
  SYN::Application app{};
  app.init(PROJECT_ROOT);
  app.run();
  app.shutdown();
}
