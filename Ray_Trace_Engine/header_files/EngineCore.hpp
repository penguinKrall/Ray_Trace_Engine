#pragma once

#include <CoreDebug.hpp>
#include <CoreDevice.hpp>
#include <CoreSwapchain.hpp>
#include <CoreWindow.hpp>
#include <Utilities_CreateObject.hpp>
#include <iostream>
#include <stb_image.h>
#include <stdio.h>
class EngineCore : public CoreDebug,
                   public CoreWindow,
                   public CoreDevice,
                   public CoreSwapchain {
public:
  // instance pointer
  EngineCore *pEngineCore = this;

  // -- vulkan object create functions
  Utilities_CreateObject objCreate;

  // -- camera
  gtp::Camera *camera;

  //// -- ui
  // CoreUI UI;

  // vulkan instance, stores all per-application states
  VkInstance instance{};
  std::string title = "vulkan_raytracing";
  std::string name = "vulkanRaytracing";
  uint32_t apiVersion = VK_API_VERSION_1_3;
  // instance extensions
  std::vector<std::string> supportedInstanceExtensions;
  std::vector<const char *> enabledExtensions;

  // window, surface
  VkSurfaceKHR surface{};

  // settings that can be changed
  struct BaseSettings {
    // activates validation layers (and message output) when set to true
    bool validation = false;
    // set to true if fullscreen mode has been requested via command line
    bool fullscreen = false;
    // set to true if v-sync will be forced for the swapchain
    bool vsync = false;
    // enable UI overlay
    bool overlay = true;
  };

  //--settings
  BaseSettings baseSettings{};

  // ctor
  EngineCore();

  // funcs
  VkInstance createInstance(bool enableValidation);

  VkSurfaceKHR createSurface();

  // -- init core
  //@brief	creates device, swapchain, command pool, queue objects
  //@brief	adds object names and handles to debug map
  void InitCore();

  // -- create buffer
  //@brief func also deals with memory ie. allocate, map, copy, unmap, bind.
  //@brief maps buffer and buffer memory handles in debug
  //@return buffer->bind(pEngineCore->devices.logical);
  VkResult CreateBuffer(VkBufferUsageFlags usageFlags,
                        VkMemoryPropertyFlags memoryPropertyFlags,
                        gtp::Buffer *buffer, VkDeviceSize size, void *data);

  VkResult CreateBuffer2(VkBufferUsageFlags usageFlags,
                         VkMemoryPropertyFlags memoryPropertyFlags,
                         VkDeviceSize size, VkBuffer *buffer,
                         VkDeviceMemory *memory, void *data = nullptr);

  // -- create queues
  //@brief creates graphics, compute, present, and transfer queues on device
  void CreateQueues();

  // -- flush command buffer
  //@brief ends recording command buffer and submits to device queue
  void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue,
                          VkCommandPool pool, bool free);

  // -- create core swapchain
  void CreateCoreSwapchain();

  // -- recreate core swapchain
  void RecreateCoreSwapchain();

  // -- create sync objects
  void CreateSyncObjects();

  // -- recreate sync objects
  void RecreateSyncObjects();

  void CreateLoadingScreenImage(const std::string &fileName);

  void LoadingScreen();

  // -- destroy core
  // destroys core related objects
  void DestroyCore();
};
