#pragma once

#include <CoreUI.hpp>
#include <EngineCore.hpp>

namespace gtp {
struct InputPosition {
  double lastX = 0.0;
  double lastY = 0.0;

  double mousePosX = 0.0;
  double mousePosY = 0.0;

  void InitializeMousePosition(float width, float height);
  glm::vec2 MousePos();
};

class Input {
private:
  // engine core ptr
  EngineCore *pEngineCore = nullptr;

  // input position data
  InputPosition inputPosition{};

  // mouse clicked on window
  bool isLMBPressed = false;

  // process input
  void ProcessInput(float deltaTime, gtp::Camera *pCamera = nullptr);

public:
  Input(EngineCore *engineCorePtr);
  glm::vec2 MousePosition();
  bool LeftMouseDown();
  void Process(float deltaTime, gtp::Camera* pCamera);
};

} // namespace gtp
