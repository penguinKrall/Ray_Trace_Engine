#include "EngineCore.hpp"

// -- ctor
EngineCore::EngineCore() {}

// -- create instance
VkInstance EngineCore::createInstance(bool enableValidation) {
  //--enable validation
  this->baseSettings.validation = enableValidation;

  //--app info
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = name.c_str();
  appInfo.pEngineName = name.c_str();
  appInfo.apiVersion = apiVersion;

  //--instance create info
  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  //--extension list
  //--get extensions supported and store in supportedInstanceExtensions
  uint32_t extCount = 0;
  validate_vk_result(
      vkEnumerateInstanceExtensionProperties(nullptr, &extCount, nullptr));

  if (extCount > 0) {
    std::vector<VkExtensionProperties> extensions(extCount);
    if (vkEnumerateInstanceExtensionProperties(
            nullptr, &extCount, extensions.data()) == VK_SUCCESS) {
      std::cout << "supported instance extensions" << std::endl;
      int extIDX = 0;
      for (const auto &extension : extensions) {
        supportedInstanceExtensions.emplace_back(extension.extensionName);
        std::cout << " [" << extIDX << "] "
                  << supportedInstanceExtensions.back() << std::endl;
        extIDX++;
      }
    }
  }

  // convert std::string to char*
  enabledExtensions.reserve(
      supportedInstanceExtensions.size()); // Reserve space for efficiency

  // fill enabled extensions
  // skip this extension bc validation layers
  for (auto &str : supportedInstanceExtensions) {
    if (!(strcmp(str.data(), "VK_LUNARG_direct_driver_loading") == 0)) {
      enabledExtensions.push_back(str.data());
    }
  }

  // reference extension list in instance create info
  instanceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(enabledExtensions.size());
  instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();

  std::cout << std::endl << "enabled instance extensions" << std::endl;
  for (int i = 0; i < enabledExtensions.size(); i++) {
    std::cout << " [" << i << "] " << enabledExtensions[i] << std::endl;
  }

  // check if validation at instance layer is supported
  if (baseSettings.validation) {
    if (enumerateProperties()) {
      instanceCreateInfo.ppEnabledLayerNames = getValidationLayerNames();
      instanceCreateInfo.enabledLayerCount = 1;
      std::cout << std::endl
                << "validation layer: " << *getValidationLayerNames()
                << "  ...initializing validation " << std::endl
                << std::endl;
    }

    else {
      std::cerr << "Validation layer VK_LAYER_KHRONOS_validation not present, "
                   "validation is disabled";
    }

    //--chain pnext to instance create info
    instanceCreateInfo.pNext = &debugMessenger.debugCreateInfo;
  }

  //--create instance
  validate_vk_result(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> layers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

  std::cout << "Available Vulkan Layers:" << std::endl;
  for (const auto &layer : layers) {
    std::cout << " - " << layer.layerName << std::endl;
  }

  std::cout << "\nsuccessfully created instance\n" << std::endl;

  return instance;
}

// -- create surface
VkSurfaceKHR EngineCore::createSurface() {

  validate_vk_result(
      glfwCreateWindowSurface(instance, windowGLFW, nullptr, &this->surface));

  return surface;
}

// -- init core
void EngineCore::InitCore() {

  // create instance
  add([this]() { return createInstance(true); }, "instance");

  // debug
  // load debug ext function pointers first
  loadFunctionPointers(instance);

  // create debug
  add([this]() { return createDebugMessenger(instance); }, "debugMessenger");

  // create GLFWwindow*
  add([this]() { return initWindow(); }, "windowGLFW");

  // create surface
  add([this]() { return createSurface(); }, "surface");

  if (getPhysicalDevice(instance) == VK_SUCCESS) {
    std::cout << std::endl
              << "main: found and selected supported GPU" << "\n"
              << std::endl;
  }

  // create logical device
  add([this]() { return createLogicalDevice(surface); }, "logicalDevice");

  // initialize camera
  camera = new gtp::Camera(glm::vec3(0.0f, 5.0f, 5.0f),
                           glm::vec3(0.0f, 1.0f, 0.0f), windowGLFW);

  // init
  // create queues
  CreateQueues();

  // Get the ray tracing and accelertion structure related function pointers
  // required by this sample
  coreExtensions->loadFunctions(devices.logical);

  // create command pool
  add(
      [this]() {
        return createCommandPool(devices.logical,
                                 queue.queueFamilyIndices.graphics);
      },
      "commandPool");

  // create compute command pool
  add(
      [this]() {
        return CreateComputeCommandPool(devices.logical,
                                        queue.queueFamilyIndices.compute);
      },
      "ComputeCommandPool");

  // init Utilities_CreateObject class with the Vulkan Object Create funcs
  // MUST be called after command pool is created
  objCreate =
      Utilities_CreateObject(&devices.physical, &devices.logical,
                             coreExtensions, pCoreDebug, commandPools.graphics);

  // create graphics command buffers
  addArray(
      [this]() {
        return createGraphicsCommandBuffers(devices.logical, devices.physical);
      },
      VkCommandBuffer{}, "graphicsCommandBuffer");

  // create command sync
  CreateSyncObjects();

  // create swapchain
  CreateCoreSwapchain();

  // loading screen image
  // this->loadingScreen =
  //    gtp::LoadingScreen(&this->devices.logical, &this->devices.physical);
  // LoadingScreen();
}

VkResult EngineCore::CreateBuffer(VkBufferUsageFlags usageFlags,
                                  VkMemoryPropertyFlags memoryPropertyFlags,
                                  gtp::Buffer *buffer, VkDeviceSize size,
                                  void *data) {

  buffer->bufferData.usageFlags = usageFlags;
  buffer->bufferData.size = size;
  buffer->bufferData.memoryPropertyFlags = memoryPropertyFlags;

  // create buffer object
  pEngineCore->AddObject(
      [this, buffer, usageFlags, size]() {
        return buffer->CreateBuffer(this->pEngineCore->devices.physical,
                                    this->pEngineCore->devices.logical,
                                    usageFlags, size,
                                    buffer->bufferData.bufferName);
      },
      buffer->bufferData.bufferName);

  // allocate buffer memory object
  pEngineCore->AddObject(
      [this, buffer]() {
        return buffer->AllocateBufferMemory(
            this->pEngineCore->devices.physical,
            this->pEngineCore->devices.logical,
            buffer->bufferData.bufferMemoryName);
      },
      buffer->bufferData.bufferMemoryName);

  buffer->bind(this->pEngineCore->devices.logical, 0);

  // if void* != null, map buffer and copy data
  if (data != nullptr) {
    // map buffer
    if (buffer->map(pEngineCore->devices.logical, size, 0) != VK_SUCCESS) {
      throw std::invalid_argument("failed to map buffer");
    }
    // copy memory to mapped buffer
    memcpy(buffer->bufferData.mapped, data, size);
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
      // flush buffer
      buffer->flush(pEngineCore->devices.logical, size, 0);

    // buffer->unmap(pEngineCore->devices.logical);
  }

  //// Initialize a default descriptor that covers the whole buffer size
  buffer->setupDescriptor();

  // Attach the memory to the buffer object
  return VK_SUCCESS;

  // return VK_SUCCESS;
}

VkResult EngineCore::CreateBuffer2(VkBufferUsageFlags usageFlags,
                                   VkMemoryPropertyFlags memoryPropertyFlags,
                                   VkDeviceSize size, VkBuffer *buffer,
                                   VkDeviceMemory *memory, void *data) {
  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.usage = usageFlags;
  bufferCreateInfo.size = size;
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(this->devices.logical, &bufferCreateInfo, nullptr, buffer);

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc{};
  memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  vkGetBufferMemoryRequirements(this->devices.logical, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  // Find a memory type index that fits the properties of the buffer
  memAlloc.memoryTypeIndex =
      getMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
  VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
  if (usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
    allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
    allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
    memAlloc.pNext = &allocFlagsInfo;
  }
  vkAllocateMemory(this->devices.logical, &memAlloc, nullptr, memory);

  // If a pointer to the buffer data has been passed, map the buffer and copy
  // over the data
  if (data != nullptr) {
    void *mapped;
    vkMapMemory(this->devices.logical, *memory, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    // If host coherency hasn't been requested, do a manual flush to make writes
    // visible
    if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0) {
      VkMappedMemoryRange mappedRange{};
      mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
      mappedRange.memory = *memory;
      mappedRange.offset = 0;
      mappedRange.size = size;
      vkFlushMappedMemoryRanges(this->devices.logical, 1, &mappedRange);
    }
    vkUnmapMemory(this->devices.logical, *memory);
  }

  // Attach the memory to the buffer object
  vkBindBufferMemory(this->devices.logical, *buffer, *memory, 0);

  return VK_SUCCESS;
}

void EngineCore::CreateQueues() {

  add(
      [this]() {
        return VKCreateQueue(devices.logical, queue.queueFamilyIndices.graphics,
                             0, &queue.graphics);
      },
      "graphicsQueue");

  add(
      [this]() {
        return VKCreateQueue(devices.logical, queue.queueFamilyIndices.compute,
                             0, &queue.compute);
      },
      "computeQueue");

  add(
      [this]() {
        return VKCreateQueue(devices.logical, queue.queueFamilyIndices.present,
                             0, &queue.present);
      },
      "presentQueue");

  add(
      [this]() {
        return VKCreateQueue(devices.logical, queue.queueFamilyIndices.transfer,
                             0, &queue.transfer);
      },
      "transferQueue");
}

void EngineCore::FlushCommandBuffer(VkCommandBuffer commandBuffer,
                                    VkQueue queue, VkCommandPool pool,
                                    bool free) {

  gtp::Utilities_EngCore::FlushCommandBuffer(this->devices.logical,
                                             commandBuffer, queue, pool, free);
}

void EngineCore::CreateCoreSwapchain() {
  // -- swapchain
  add(
      [this]() {
        return createSwapchain(devices.physical, devices.logical, surface,
                               windowGLFW, queue.queueFamilyIndices);
      },
      "swapchain");

  // -- swapchain images
  addArray([this]() { return createSwapchainImage(devices.logical); },
           VkImage{}, "swapchainImage");

  // -- swapchain imageViews
  addArray([this]() { return createSwapchainImageView(devices.logical); },
           VkImageView{}, "swapchainImageView");
}

void EngineCore::RecreateCoreSwapchain() {

  // destroy swapchain image views
  for (const auto &swapchainimageviews :
       swapchainData.swapchainImages.imageView) {
    vkDestroyImageView(devices.logical, swapchainimageviews, nullptr);
  }

  // destroy swapchain
  vkDestroySwapchainKHR(devices.logical, swapchainData.swapchainKHR, nullptr);

  // -- swapchain
  add(
      [this]() {
        return createSwapchain(devices.physical, devices.logical, surface,
                               windowGLFW, queue.queueFamilyIndices);
      },
      "swapchain");

  // -- swapchain images
  addArray([this]() { return createSwapchainImage(devices.logical); },
           VkImage{}, "swapchainImage");

  // VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0,
  // 1, 0, 1 };

  // create temporary command buffer for transition/copy
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  for (const auto &swpchn : swapchainData.swapchainImages.image) {
    // transition image layout
    gtp::Utilities_EngCore::setImageLayout(
        cmdBuffer, swpchn, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
  }

  // submit temporary command buffer
  pEngineCore->FlushCommandBuffer(cmdBuffer, queue.graphics,
                                  commandPools.graphics, true);

  // -- swapchain imageViews
  addArray([this]() { return createSwapchainImageView(devices.logical); },
           VkImageView{}, "swapchainImageView");
}

void EngineCore::CreateSyncObjects() {
  // semaphores
  // add([this]() {
  //	return CreatePresentSemaphore(devices.logical); },
  //"PresentFinishedSemaphore");
  //
  // add([this]() {
  //	return CreateRenderSemaphore(devices.logical); },
  //"RenderFinishedSemaphore");
  //
  // add([this]() {
  //	return CreateComputeSemaphore(devices.logical); }, "ComputeSemaphore");

  // semaphores
  addArray([this]() { return CreatePresentSemaphore(devices.logical); },
           VkSemaphore{}, "presentSemaphores");

  addArray([this]() { return CreateRenderSemaphore(devices.logical); },
           VkSemaphore{}, "renderFinishedSemaphores");

  addArray([this]() { return CreateComputeSemaphore(devices.logical); },
           VkSemaphore{}, "computeSemaphores");

  // fences
  addArray([this]() { return CreateFences(devices.logical); }, VkFence{},
           "syncFences");

  // fences
  addArray([this]() { return CreateComputeFences(devices.logical); }, VkFence{},
           "syncComputeFences");
}

void EngineCore::RecreateSyncObjects() {

  // destroy
  //--sync
  // fences
  for (const auto &fence : sync.drawFences) {
    vkDestroyFence(this->devices.logical, fence, nullptr);
  }

  for (const auto &fence : sync.computeFences) {
    vkDestroyFence(this->devices.logical, fence, nullptr);
  }

  // semaphores
  for (int i = 0; i < frame_draws; i++) {
    vkDestroySemaphore(this->devices.logical, sync.renderFinishedSemaphore[i],
                       nullptr);
    vkDestroySemaphore(this->devices.logical, sync.presentFinishedSemaphore[i],
                       nullptr);
    vkDestroySemaphore(this->devices.logical, sync.computeFinishedSemaphore[i],
                       nullptr);
  }

  // create
  CreateSyncObjects();
}

// void EngineCore::CreateLoadingScreenImage(const std::string &fileName) {
//
//   int width;
//   int height;
//   int channels;
//   VkDeviceSize imageSize;
//
//   std::string fileLoc =
//       "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/" +
//       fileName;
//   stbi_uc *image =
//       stbi_load(fileLoc.c_str(), &width, &height, &channels, STBI_rgb_alpha);
//
//   if (!image) {
//     throw std::invalid_argument("Failed to load Texture!(" + fileName + ")");
//   }
//
//   imageSize = static_cast<VkDeviceSize>(width) * height * 4;
//
//   gtp::Buffer stagingBuffer;
//
//   stagingBuffer.bufferData.bufferName = "load_screen_image_staging_buffer";
//   stagingBuffer.bufferData.bufferMemoryName =
//       "load_screen_image_staging_bufferMemory";
//
//   validate_vk_result(
//       pEngineCore->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
//                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                                 &stagingBuffer, imageSize, image));
//
//   VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
//   VkImage srcImage;
//   VkDeviceMemory srcImageMemory;
//
//   VkMemoryRequirements memReqs{};
//
//   VkMemoryAllocateInfo memAllocInfo{};
//   memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//
//   VkImageCreateInfo imageCreateInfo{};
//   imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//   imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
//   imageCreateInfo.format =
//       this->swapchainData.assignedSwapchainImageFormat.format;
//   imageCreateInfo.mipLevels = 1;
//   imageCreateInfo.arrayLayers = 1;
//   imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//   imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
//   imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
//   imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//   imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//   imageCreateInfo.extent = {static_cast<uint32_t>(width),
//                             static_cast<uint32_t>(height), 1};
//   imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
//                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
//                           VK_IMAGE_USAGE_SAMPLED_BIT;
//
//   validate_vk_result(vkCreateImage(pEngineCore->devices.logical,
//                                    &imageCreateInfo, nullptr, &srcImage));
//
//   vkGetImageMemoryRequirements(pEngineCore->devices.logical, srcImage,
//                                &memReqs);
//   memAllocInfo.allocationSize = memReqs.size;
//   memAllocInfo.memoryTypeIndex = pEngineCore->getMemoryType(
//       memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//   if (vkAllocateMemory(pEngineCore->devices.logical, &memAllocInfo, nullptr,
//                        &srcImageMemory) != VK_SUCCESS) {
//     std::cerr << "\nfailed to allocate image memory in EngineCore";
//   }
//   if (vkBindImageMemory(pEngineCore->devices.logical, srcImage,
//   srcImageMemory,
//                         0) != VK_SUCCESS) {
//     std::cerr << "\nfailed to bind image memory in EngineCore";
//   }
//
//   VkImageSubresourceRange subresourceRange{};
//   subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//   subresourceRange.baseMipLevel = 0;
//   subresourceRange.levelCount = 1;
//   subresourceRange.baseArrayLayer = 0;
//   subresourceRange.layerCount = 1;
//
//   // create command buffer
//   VkCommandBuffer commandBuffer =
//   pEngineCore->objCreate.VKCreateCommandBuffer(
//       VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//
//   VkBufferImageCopy imageRegion = {};
//   imageRegion.bufferOffset = 0;
//   imageRegion.bufferRowLength = 0;
//   imageRegion.bufferImageHeight = 0;
//   imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//   imageRegion.imageSubresource.mipLevel = 0;
//   imageRegion.imageSubresource.baseArrayLayer = 0;
//   imageRegion.imageSubresource.layerCount = 1;
//   imageRegion.imageOffset = {
//       0,
//       0,
//       0,
//   };
//   imageRegion.imageExtent = {static_cast<uint32_t>(width),
//                              static_cast<uint32_t>(height), 1};
//
//   gtp::Utilities_EngCore::setImageLayout(
//       commandBuffer, srcImage, VK_IMAGE_LAYOUT_UNDEFINED,
//       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
//
//   // copy buffer to image
//   vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.bufferData.buffer,
//                          srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
//                          &imageRegion);
//
//   gtp::Utilities_EngCore::setImageLayout(
//       commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);
//
//   for (int i = 0; i < this->swapchainData.swapchainImages.image.size(); i++)
//   {
//
//     gtp::Utilities_EngCore::setImageLayout(
//         commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
//         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//         subresourceRange);
//
//     VkImageCopy copyRegion{};
//     copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
//     copyRegion.srcOffset = {0, 0, 0};
//     copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
//     copyRegion.dstOffset = {0, 0, 0};
//     copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
//                          pEngineCore->swapchainData.swapchainExtent2D.height,
//                          1};
//
//     vkCmdCopyImage(commandBuffer, srcImage,
//                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                    pEngineCore->swapchainData.swapchainImages.image[i],
//                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);
//
//     // for (int i = 0; i < this->swapchainData.swapchainImages.image.size();
//     // i++) {
//     gtp::Utilities_EngCore::setImageLayout(
//         commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
//         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
//         subresourceRange);
//     //}
//
//     // for (int i = 0; i < this->swapchainData.swapchainImages.image.size();
//     // i++) {
//     gtp::Utilities_EngCore::setImageLayout(
//         commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
//         VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
//         subresourceRange);
//     //}
//   }
//
//   // flush one time submit command buffer
//   this->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
//                            pEngineCore->commandPools.graphics, true);
//
//   // Destroy staging buffers
//   stagingBuffer.destroy(this->devices.logical);
//   vkDestroyImage(this->devices.logical, srcImage, nullptr);
//   vkFreeMemory(this->devices.logical, srcImageMemory, nullptr);
// }
//
// void EngineCore::LoadingScreen() {
//   // -- -- -- -- LOADING SCREEN  -- -- -- -- //
//
//   CreateLoadingScreenImage("gondola_proj/loading_800x600.png");
//
//   VkImageSubresourceRange subresourceRange{};
//   subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//   subresourceRange.baseMipLevel = 0;
//   subresourceRange.levelCount = 1;
//   subresourceRange.baseArrayLayer = 0;
//   subresourceRange.layerCount = 1;
//
//   // VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
//
//   uint32_t imageIndex = 0;
//
//   // wait for draw fences
//   vkWaitForFences(devices.logical, 1, &sync.drawFences[0], VK_TRUE,
//                   std::numeric_limits<uint64_t>::max());
//
//   vkResetFences(devices.logical, 1, &sync.drawFences[0]);
//
//   // acquire next image
//   VkResult acquireImageResult = vkAcquireNextImageKHR(
//       devices.logical, swapchainData.swapchainKHR, UINT64_MAX,
//       sync.presentFinishedSemaphore[0], VK_NULL_HANDLE, &imageIndex);
//
//   // create command buffer
//   VkCommandBuffer commandBuffer =
//   pEngineCore->objCreate.VKCreateCommandBuffer(
//       VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//
//   // end and submit command buffer
//   vkEndCommandBuffer(commandBuffer);
//
//   // pipeline wait stages
//   std::vector<VkPipelineStageFlags> waitStages = {
//       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
//
//   std::vector<VkSemaphore> graphicsSemaphores = {
//       sync.presentFinishedSemaphore[0]};
//
//   VkSubmitInfo submitInfo{};
//   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//   submitInfo.pWaitDstStageMask = waitStages.data();
//   submitInfo.waitSemaphoreCount =
//       static_cast<uint32_t>(graphicsSemaphores.size());
//   submitInfo.pWaitSemaphores = graphicsSemaphores.data();
//   submitInfo.signalSemaphoreCount = 1;
//   submitInfo.pSignalSemaphores = &sync.renderFinishedSemaphore[0];
//   submitInfo.commandBufferCount = 1;
//   submitInfo.pCommandBuffers = &commandBuffer;
//
//   // Submit command buffers to the graphics queue
//   validate_vk_result(
//       vkQueueSubmit(queue.graphics, 1, &submitInfo, sync.drawFences[0]));
//
//   // Prepare present info for swapchain presentation
//   VkPresentInfoKHR presentInfo = {};
//   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//   presentInfo.waitSemaphoreCount = 1;
//   presentInfo.pWaitSemaphores = &sync.renderFinishedSemaphore[0];
//   submitInfo.signalSemaphoreCount = 1;
//   submitInfo.pSignalSemaphores = &sync.presentFinishedSemaphore[0];
//   presentInfo.swapchainCount = 1;
//   presentInfo.pSwapchains = &swapchainData.swapchainKHR;
//   presentInfo.pImageIndices = &imageIndex;
//
//   // Present the rendered image to the screen
//   VkResult presentImageResult = vkQueuePresentKHR(queue.graphics,
//   &presentInfo);
//
//   // -- -- -- -- END LOADING SCREEN  -- -- -- -- //
// }

//void EngineCore::InitXYPos()
//{
//  
//    lastX = float(this->GetWindowDimensions().width) / 2.0f;
//    lastY = float(this->GetWindowDimensions().height) / 2.0f;
//
//    posX = float(this->GetWindowDimensions().width) / 2.0f;
//    posY = float(this->GetWindowDimensions().height) / 2.0f;
//  
//}

// -- destroy
void EngineCore::DestroyCore() {
  // swapchain
  DestroySwapchain(devices.logical);

  // surface
  vkDestroySurfaceKHR(instance, surface, nullptr);

  // logical device
  destroyDevice();

  // debug
  destroyDebug(instance);

  // instance
  vkDestroyInstance(instance, nullptr);

  // window
  destroyWindow();
}

GLFWwindow* EngineCore::CoreGLFWwindow()
{
  return this->windowGLFW;
}
