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
public:
  int width = 0;
  int height = 0;

  CoreWindow();

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
  void destroyWindow();
};
