#pragma once

#include <CoreDebug.hpp>
#include <CoreDevice.hpp>
#include <CoreSwapchain.hpp>
#include <CoreWindow.hpp>
#include <Utilities_CreateObject.hpp>
#include <stb_image.h>
// #include <LoadingScreen.hpp>

class EngineCore : private CoreDebug,
                   private CoreWindow,
                   public CoreDevice,
                   public CoreSwapchain {
private:
  // gtp::LoadingScreen loadingScreen;

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
    bool validation = true;
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

  uint64_t GetBufferDeviceAddress(VkBuffer buffer) {
    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = buffer;
    return coreExtensions->vkGetBufferDeviceAddressKHR(
        pEngineCore->devices.logical, &bufferDeviceAI);
  }

  //void InitXYPos();

  //get logical device
  VkDevice LogicalDevice();

  VkCommandBuffer GraphicsCommandBuffer(int frame);

  // handle compute fence wait/reset
  void ComputeFence(int frame);
  
  // submit compute queue
  void SubmitComputeQueue(VkSubmitInfo computeSubmitInfo, int frame);

  //return current compute semaphore
  VkSemaphore ComputeSemaphore(int frame);

  //return current draw semaphore
  VkSemaphore RenderFinishedSemaphore(int frame);

  //return current present semaphore
  VkSemaphore PresentSemaphore(int frame);

  //acquire image
  VkResult AcquireImageResult(int frame, uint32_t* imageIndex);

  //handle draw fence wait/reset
  void DrawFence(int frame);

  // submit graphics queue
  void SubmitGraphicsQueue(VkSubmitInfo submitInfo, int frame);

  //present queue
  void PresentQueue(VkPresentInfoKHR presentInfo, int frame);

  // -- destroy core
  // destroys core related objects
  void DestroyCore();

  GLFWwindow *CoreGLFWwindow();
  template <typename Func>
  inline void AddObject(Func createFunc, const std::string &name);
};

template <typename Func>
inline void EngineCore::AddObject(Func createFunc, const std::string &name) {
  this->pCoreDebug->add(createFunc, name);
}
