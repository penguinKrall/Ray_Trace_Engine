#pragma once

#include <Utilities_EngCore.hpp>

// #include <vulkan/vulkan.h>
// #include <GLFW/glfw3.h>
// #include <GLFW/glfw3native.h>

// #include <vector>
// #include <array>
// #include <string>
//
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <assert.h>
// #include <vector>
// #include <array>
// #include <unordered_map>
// #include <numeric>
// #include <ctime>
// #include <iostream>
// #include <chrono>
// #include <random>
// #include <algorithm>
// #include <sys/stat.h>
// #include <set>

class CoreWindow {
private:
  VkExtent3D windowExtent = {800, 600, 1};

public:
  //int width = 0;
  //int height = 0;

  CoreWindow();

  //double lastX = 0.0f;
  //double lastY = 0.0f;

  //double posX = 0.0f;
  //double posY = 0.0f;

  VkClearValue colorClearValue{};
  VkClearValue depthClearValue{};

  VkOffset2D offset{};
  std::vector<VkDeviceSize> offsets{};

  VkExtent2D extent{};
  VkRect2D renderArea{};
  VkViewport viewport{};
  VkRect2D scissor{};

  // Frame counter to display fps
  uint32_t frameCounter = 0;
  uint32_t lastFPS = 0;

  std::chrono::time_point<std::chrono::high_resolution_clock> lastTimestamp,
      tPrevEnd;

  GLFWwindow *windowGLFW = nullptr;

  // funcs
  GLFWwindow *
  initWindow(std::string wName = "vulkan_raytracing project window");

  // get window dimensions
  //@brief returns vkextent3d - width/height/depth uint32
  VkExtent3D GetWindowDimensions();
  void SetWindowDimensions(uint32_t width = 800, uint32_t height = 600, uint32_t depth = 1);

  void destroyWindow();
};
