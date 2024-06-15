#include "main.hpp"

int main() {

  // engine contains core components for device/window/swapchain creation
  auto engine = new gtp::Engine();

  // init
  engine->InitEngine();

  // run
  engine->Run();

  engine->Terminate();
}
