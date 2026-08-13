#include "SynoraEngine/core/Application.h"

int main() {
  SYN::Application app{};
  app.init();
  app.run();
  app.shutdown();
}
