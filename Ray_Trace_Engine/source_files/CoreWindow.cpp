#include "CoreWindow.hpp"

// -- ctor
CoreWindow::CoreWindow() {}

// -- init window
GLFWwindow *CoreWindow::initWindow(std::string wName) {

  // GLFWwindow* newWindow = nullptr;

  glfwInit();

  //// Get the primary monitor (you can also get a specific monitor)
  // GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
  // if (!primaryMonitor) {
  //	glfwTerminate();
  //	return nullptr;
  // }
  //
  //// Get the video mode of the monitor
  // const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
  // if (!mode) {
  //	glfwTerminate();
  //	return nullptr;
  // }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); //no border

  this->width = 800;
  this->height = 600;

  windowGLFW =
      glfwCreateWindow(width, height, "Fullscreen Window", nullptr, nullptr);

  VkResult result = VK_ERROR_EXTENSION_NOT_PRESENT;

  if (windowGLFW != nullptr) {
    result = VK_SUCCESS;
  }

  return windowGLFW;
}

void CoreWindow::destroyWindow() { glfwDestroyWindow(windowGLFW); }
