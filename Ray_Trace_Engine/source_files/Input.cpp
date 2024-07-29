#include "Input.hpp"

void gtp::InputPosition::InitializeMousePosition(float width, float height)
{
    lastX = width / 2.0f;
    lastY = height / 2.0f;

    mousePosX = width / 2.0f;
    mousePosY = height / 2.0f;
  
}

glm::vec2 gtp::InputPosition::MousePos() {
  return glm::vec2(static_cast<float>(this->mousePosX),
                   static_cast<float>(this->mousePosY));
}

void gtp::Input::ProcessInput(float deltaTime, gtp::Camera *pCamera) {
  if (pCamera != nullptr) {
    // float deltaTime = deltaTime;
    glfwGetCursorPos(this->pEngineCore->CoreGLFWwindow(),
      &inputPosition.mousePosX, &inputPosition.mousePosY);

    // Print mouse coordinates (you can use them as needed)
    // printf("Mouse Coordinates: %.2f, %.2f\n", mousePosX, mousePosY);

    auto xoffset =
      static_cast<float>(inputPosition.mousePosX - inputPosition.lastX);
    auto yoffset =
      static_cast<float>(inputPosition.lastY - inputPosition.mousePosY);

    inputPosition.lastX = inputPosition.mousePosX;
    inputPosition.lastY = inputPosition.mousePosY;

    if (glfwGetMouseButton(this->pEngineCore->CoreGLFWwindow(),
      GLFW_MOUSE_BUTTON_RIGHT) == GLFW_TRUE) {
      pCamera->viewUpdated = true;
      pCamera->ProcessMouseMovement(xoffset, yoffset);
    }

    if (glfwGetMouseButton(this->pEngineCore->CoreGLFWwindow(),
      GLFW_MOUSE_BUTTON_LEFT) == GLFW_TRUE) {
      this->isLMBPressed = true;
    }
    if (glfwGetMouseButton(this->pEngineCore->CoreGLFWwindow(),
      GLFW_MOUSE_BUTTON_LEFT) == GLFW_FALSE) {
      this->isLMBPressed = false;
    }

    // if (glfwGetKey(CoreGLFWwindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    //   pCamera->viewUpdated = true;
    //   glfwSetWindowShouldClose(CoreGLFWwindow(), true);
    // }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_W) ==
      GLFW_PRESS) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::FORWARD, deltaTime);
    }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_S) ==
      GLFW_PRESS) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::BACKWARD, deltaTime);
    }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_A) ==
      GLFW_PRESS) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::LEFT, deltaTime);
    }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_D) ==
      GLFW_PRESS) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::RIGHT, deltaTime);
    }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_UP) ==
      GLFW_TRUE) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::UP, deltaTime);
    }

    if (glfwGetKey(this->pEngineCore->CoreGLFWwindow(), GLFW_KEY_DOWN) ==
      GLFW_TRUE) {
      pCamera->viewUpdated = true;
      pCamera->ProcessKeyboard(Movement::DOWN, deltaTime);
    }
  }
}

gtp::Input::Input(EngineCore *engineCorePtr) : pEngineCore(engineCorePtr) {
  // init xy pos
  this->inputPosition.InitializeMousePosition(800.0f, 600.0f);
}

glm::vec2 gtp::Input::MousePosition()
{
  return this->inputPosition.MousePos();
}

bool gtp::Input::LeftMouseDown()
{
  return this->isLMBPressed;
}

void gtp::Input::Process(float deltaTime, gtp::Camera* pCamera)
{
  this->ProcessInput(deltaTime, pCamera);
}
