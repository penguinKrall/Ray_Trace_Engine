#pragma once

// #define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#include <array>
#include <string>
#include <vector>

// #include <stdio.h>
// #include <stdlib.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <assert.h>
#include <string.h>
#include <vulkan/vulkan.h>

#include <Buffer.hpp>
#include <Camera.hpp>
#include <Utilities_EnumStringHelper.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <unordered_map>
#include <vector>

#define validate_vk_result(f)                                                  \
  do {                                                                         \
    VkResult res = (f);                                                        \
    if (res != VK_SUCCESS) {                                                   \
      std::cout << "error! VkResult == " << res << " in " << __FILE__          \
                << " at line " << __LINE__ << "\n";                            \
      assert(res == VK_SUCCESS);                                               \
    }                                                                          \
  } while (0)

const int frame_draws = 3;

// -- vulkan ray tracing
//@brief i'm new to namespaces..
namespace gtp {

// -- tools class --
//@brief static functions used by all levels of program
class Utilities_EngCore {
public:
  // queue family indices
  struct QueueFamilyIndices {
    int graphics = -1;
    int compute = -1;
    int present = -1;
    int transfer = -1;
  };

  struct ImageData {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkSampler sampler;
    int width;
    int height;
    int channels;
    VkDeviceSize imageSize;
    std::string name;
  };

  // -- get working directory
  static std::string BuildPath(const std::string &fileName);

  static std::vector<std::string>
  ListFilesInDirectory(const std::string &directory) {
    // list of files
    std::vector<std::string> fileList;

    // iterate through directory
    std::filesystem::path path(directory);
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
      for (const auto &entry : std::filesystem::directory_iterator(path)) {
        std::cout << "ListFilesInDirectory()_\n\tentry.path().string(): "
                  << entry.path().string() << std::endl;
        fileList.push_back(entry.path().string());
      }
    } else {
      std::cout << "Directory does not exist or is not a directory."
                << std::endl;
    }

    // return file list
    return fileList;
  }

  // -- get queue family indices
  static QueueFamilyIndices
  getQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

  // -- validate queue family indices
  static bool validateQueueFamilyIndices(QueueFamilyIndices queueFamily);

  // -- output surface capabilities
  //@brief output surface capabilities data to console for debugging
  static void
  outputSurfaceCapabilities(const VkSurfaceCapabilitiesKHR &capabilities);

  // -- physical device type to string
  //@brief output device data to console for debugging
  static std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
  static bool loadFunctionPointer(PFN_vkVoidFunction &functionPointer,
                                  VkDevice logicalDevice,
                                  const char *functionName);
  static bool loadFunctionPointer(PFN_vkVoidFunction &functionPointer,
                                  VkInstance instance,
                                  const char *functionName);

  // -- set image layout
  //@brief create an image memory barrier for changing the layout of brief an
  // image and put it into an active command buffer
  static void setImageLayout(
      VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
      VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
      VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
      VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  // -- aligned size(uint32_t)
  static uint32_t alignedSize(uint32_t value, uint32_t alignment);

  // -- aligned size(size_t)
  static size_t alignedSize(size_t value, size_t alignment);

  // copied this direct from sacha to make part of gltf model work when iw as
  // being tired/lazy fix up to be uniform..
  struct VkInitializers {
    // -- descriptor set layout binding
    //@brief saves some space by returning an initialized struct of
    // VkDescriptorSetLayoutBinding
    static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding(
        uint32_t binding, VkDescriptorType descriptorType,
        uint32_t descriptorCount, VkShaderStageFlags stageFlags,
        const VkSampler *pImmutableSamplers);
  };

  static void FlushCommandBuffer(VkDevice logicalDevice,
                                 VkCommandBuffer commandBuffer, VkQueue queue,
                                 VkCommandPool pool, bool free);
};

} // namespace gtp
