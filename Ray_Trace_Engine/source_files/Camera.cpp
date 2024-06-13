#include "Camera.hpp"

namespace gtp {

Camera::Camera(){};

Camera::Camera(glm::vec3 position, glm::vec3 up, GLFWwindow *pWindow) {
  this->Position = position;
  this->WorldUp = up;
  this->Yaw = YAW;
  this->Pitch = PITCH;
  window = pWindow;
  this->Zoom = ZOOM;
  this->MouseSensitivity = SENSITIVITY;
  this->MovementSpeed = SPEED;
  // this->Front = glm::vec3(0.0f, 0.0f, 1.0f);
  activeWindow = window;
  viewUpdated = true;
  updateCameraVectors();

  // Store the pointer to the class instance with the GLFW window
  glfwSetWindowUserPointer(window, this);

  // key callback
  glfwSetKeyCallback(window, keyCallbackStatic);

  // Set the scroll callback
  glfwSetScrollCallback(window, Camera::staticScrollCallback);

  // set focus callback
  glfwSetWindowFocusCallback(window, Camera::windowFocusCallbackStatic);

  // framebuffer size callback
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallbackStatic);
}

glm::mat4 Camera::GetViewMatrix() {
  return lookAt(this->Position, this->Position + this->Front, this->Up);
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset) {

  xoffset *= this->MouseSensitivity;
  yoffset *= this->MouseSensitivity;

  this->Yaw += xoffset;
  this->Pitch += yoffset;

  if (this->Pitch >= 89.0f)
    this->Pitch = 89.0f;
  if (this->Pitch <= -89.0f)
    this->Pitch = -89.0f;

  // ProcessMouseScroll(yoffset);

  updateCameraVectors();
}

void Camera::ProcessKeyboard(Movement direction, float deltaTime) {
  const float velocity = this->MovementSpeed * deltaTime;
  if (direction == Movement::FORWARD)
    this->Position += this->Front * velocity;
  if (direction == Movement::BACKWARD)
    this->Position -= this->Front * velocity;
  if (direction == Movement::RIGHT)
    this->Position += this->Right * velocity;
  if (direction == Movement::LEFT)
    this->Position -= this->Right * velocity;
  if (direction == Movement::UP)
    this->Position += this->Up * velocity;
  if (direction == Movement::DOWN)
    this->Position -= this->Up * velocity;
}

void Camera::ProcessMouseScroll(float yoffset) {
  this->Zoom -= float(yoffset);
  if (Zoom >= 45.0f)
    Zoom = 45.0f;
  if (Zoom <= 20.0f)
    Zoom = 20.0f;
}

void Camera::updateCameraVectors() {
  // Calculate the new front vector
  glm::vec3 front = Front;
  front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
  front.y = sin(glm::radians(this->Pitch));
  front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
  this->Front = front;

  // Re-calculate the right and up vectors
  this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
  this->Up = glm::normalize(glm::cross(this->Right, this->Front));
}

void Camera::keyCallbackStatic(GLFWwindow *window, int key, int scancode,
                               int action, int mods) {
  Camera *instance = static_cast<Camera *>(glfwGetWindowUserPointer(window));

  if (instance) {
    instance->viewUpdated = true;
  }
}

void Camera::windowFocusCallbackStatic(GLFWwindow *focusWindow, int focused) {
  // Get the instance of VulkanWindow associated with the GLFW window
  Camera *instance =
      static_cast<Camera *>(glfwGetWindowUserPointer(focusWindow));

  if (instance) {
    // Call the non-static member function
    instance->windowFocusCallback(focusWindow, focused);
  }
}

void Camera::windowFocusCallback(GLFWwindow *newwindow, int focused) {
  if (focused != 0) {
    // The window has gained focus
    activeWindow = window;
    std::cout
        << "+++++++++++++++++++++++++++++++++++++++++++++++++++Window focused."
        << std::endl;
  } else {
    // The window has lost focus
    activeWindow = nullptr;
    std::cout << std::endl << "Window unfocused." << std::endl;
  }
}

void Camera::framebufferResizeCallbackStatic(GLFWwindow *fbufWindow, int width,
                                             int height) {
  Camera *instance =
      static_cast<Camera *>(glfwGetWindowUserPointer(fbufWindow));
  if (instance) {
    // Call the non-static member function
    instance->framebufferResizeCallback(fbufWindow, width, height);
  }
}

void Camera::framebufferResizeCallback(GLFWwindow *fbufWindow, int width,
                                       int height) {

  if (!framebufferResized) {
    std::cout << std::endl << "framebuffer resized " << std::endl;
  }

  framebufferResized = true;
}

void Camera::staticScrollCallback(GLFWwindow *window, double xoffset,
                                  double yoffset) {
  // Retrieve the class instance from the GLFW window
  Camera *instance = static_cast<Camera *>(glfwGetWindowUserPointer(window));
  if (instance) {
    // Forward the static callback to the class instance
    instance->scrollCallback(xoffset, yoffset);
    instance->viewUpdated = true;
  }
}

void Camera::scrollCallback(double xoffset, double yoffset) {
  ProcessMouseScroll(static_cast<float>(yoffset));
}

} // namespace gtp
