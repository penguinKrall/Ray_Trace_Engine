#include "main.hpp"
#include <memory>

int main() {
  auto createAndRunEngine = []() {
    auto engine = std::make_unique<gtp::Engine>();
    engine->InitEngine();
    engine->Run();
    engine->Terminate();
  };

  // Call the lambda
  createAndRunEngine();
  return 0;
}
