#include "LoadingScreen.hpp"

void gtp::LoadingScreen::InitLoadingScreen(EngineCore *engineCorePtr) {
  // init core pointer
  this->pEngineCore = engineCorePtr;

  // create loading screen image
  this->CreateLoadingScreenImage("gondola_proj/loading_800x600.png");

  // output success to console
  std::cout << "\nsuccessfully created loading screen class" << std::endl;
}

void gtp::LoadingScreen::CreateLoadingScreenImage(const std::string &fileName) {
  int width;
  int height;
  int channels;
  VkDeviceSize imageSize;

  // std::string fileLoc =
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/" + fileName;

  std::cout << "loading screen image file name: "
            << gtp::Utilities_EngCore::BuildPath(fileName).c_str() << std::endl;

  stbi_uc *image =
      stbi_load(gtp::Utilities_EngCore::BuildPath(fileName).c_str(), &width,
                &height, &channels, STBI_rgb_alpha);

  if (!image) {
    throw std::invalid_argument("Failed to load Texture!(" + fileName + ")");
  }

  imageSize = static_cast<VkDeviceSize>(width) * height * 4;

  gtp::Buffer stagingBuffer;

  stagingBuffer.bufferData.bufferName = "load_screen_image_staging_buffer";
  stagingBuffer.bufferData.bufferMemoryName =
      "load_screen_image_staging_bufferMemory";

  validate_vk_result(
      pEngineCore->CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &stagingBuffer, imageSize, image));

  VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
  VkImage srcImage;
  VkDeviceMemory srcImageMemory;

  VkMemoryRequirements memReqs{};

  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format =
      this->pEngineCore->swapchainData.assignedSwapchainImageFormat.format;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
  imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageCreateInfo.extent = {static_cast<uint32_t>(width),
                            static_cast<uint32_t>(height), 1};
  imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                          VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                          VK_IMAGE_USAGE_SAMPLED_BIT;

  validate_vk_result(vkCreateImage(pEngineCore->devices.logical,
                                   &imageCreateInfo, nullptr, &srcImage));

  vkGetImageMemoryRequirements(pEngineCore->devices.logical, srcImage,
                               &memReqs);
  memAllocInfo.allocationSize = memReqs.size;
  memAllocInfo.memoryTypeIndex = pEngineCore->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  if (vkAllocateMemory(pEngineCore->devices.logical, &memAllocInfo, nullptr,
                       &srcImageMemory) != VK_SUCCESS) {
    std::cerr << "\nfailed to allocate image memory in EngineCore";
  }
  if (vkBindImageMemory(pEngineCore->devices.logical, srcImage, srcImageMemory,
                        0) != VK_SUCCESS) {
    std::cerr << "\nfailed to bind image memory in EngineCore";
  }

  VkImageSubresourceRange subresourceRange{};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = 1;

  // create command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  VkBufferImageCopy imageRegion = {};
  imageRegion.bufferOffset = 0;
  imageRegion.bufferRowLength = 0;
  imageRegion.bufferImageHeight = 0;
  imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageRegion.imageSubresource.mipLevel = 0;
  imageRegion.imageSubresource.baseArrayLayer = 0;
  imageRegion.imageSubresource.layerCount = 1;
  imageRegion.imageOffset = {
      0,
      0,
      0,
  };
  imageRegion.imageExtent = {static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height), 1};

  gtp::Utilities_EngCore::setImageLayout(
      commandBuffer, srcImage, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

  // copy buffer to image
  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.bufferData.buffer,
                         srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                         &imageRegion);

  gtp::Utilities_EngCore::setImageLayout(
      commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

  for (int i = 0;
       i < this->pEngineCore->swapchainData.swapchainImages.image.size(); i++) {

    gtp::Utilities_EngCore::setImageLayout(
        commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                         pEngineCore->swapchainData.swapchainExtent2D.height,
                         1};

    vkCmdCopyImage(commandBuffer, srcImage,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   pEngineCore->swapchainData.swapchainImages.image[i],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // for (int i = 0; i < this->swapchainData.swapchainImages.image.size();
    // i++) {
    gtp::Utilities_EngCore::setImageLayout(
        commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);
    //}

    // for (int i = 0; i < this->swapchainData.swapchainImages.image.size();
    // i++) {
    gtp::Utilities_EngCore::setImageLayout(
        commandBuffer, pEngineCore->swapchainData.swapchainImages.image[i],
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        subresourceRange);
    //}
  }

  // flush one time submit command buffer
  this->pEngineCore->FlushCommandBuffer(
      commandBuffer, pEngineCore->queue.graphics,
      pEngineCore->commandPools.graphics, true);

  // Destroy staging buffers
  stagingBuffer.destroy(this->pEngineCore->GetLogicalDevice());
  vkDestroyImage(this->pEngineCore->GetLogicalDevice(), srcImage, nullptr);
  vkFreeMemory(this->pEngineCore->GetLogicalDevice(), srcImageMemory, nullptr);
}

void gtp::LoadingScreen::DrawLoadingScreen(CoreUI uiPtr) {

  VkImageSubresourceRange subresourceRange{};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = 1;

  // VkClearColorValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };

  uint32_t imageIndex = 0;

  // wait for draw fences
  vkWaitForFences(this->pEngineCore->GetLogicalDevice(), 1,
                  &this->pEngineCore->sync.drawFences[0], VK_TRUE,
                  std::numeric_limits<uint64_t>::max());

  vkResetFences(this->pEngineCore->GetLogicalDevice(), 1,
                &this->pEngineCore->sync.drawFences[0]);

  // acquire next image
  VkResult acquireImageResult = vkAcquireNextImageKHR(
      this->pEngineCore->GetLogicalDevice(),
      this->pEngineCore->swapchainData.swapchainKHR, UINT64_MAX,
      this->pEngineCore->sync.presentFinishedSemaphore[0], VK_NULL_HANDLE,
      &imageIndex);

  // create command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  uiPtr.backends.io = &ImGui::GetIO();

  // file/menu on top left corner of window
  ImGui::Begin("topFrameBar", 0,
    ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

  ImGui::SetWindowPos(ImVec2(0, 0)); // set position to top left
  ImGui::SetWindowSize(ImVec2(
    static_cast<float>(pEngineCore->swapchainData.swapchainExtent2D.width),
    100)); // set size of top left window

  // top left window menu bar
  // file/close
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File", true)) {

      if (ImGui::MenuItem("Exit", "Esc")) {
        glfwSetWindowShouldClose(pEngineCore->windowGLFW, true);
      }

      ImGui::EndMenu(); // End "File" drop-down menu
    }

    // Place this line after rendering the menu bar
    // ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("
    // framerate: 000.000 ms/frame ( 0.0 FPS )  ").x);

    // Then render your text
    ImGui::Text("framerate: %.3f ms/frame (%.1f FPS)",
      1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::EndMenuBar(); // End main menu bar
  }

  ImGui::End();

  ImGui::Render();

  uiPtr.backends.drawData = ImGui::GetDrawData();

  // update UI vertex/index buffers
  uiPtr.UpdateBuffers();

  // draw UI
  uiPtr.DrawUI(commandBuffer, 0);

  // end and submit command buffer
  vkEndCommandBuffer(commandBuffer);

  // pipeline wait stages
  std::vector<VkPipelineStageFlags> waitStages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  std::vector<VkSemaphore> graphicsSemaphores = {
      this->pEngineCore->sync.presentFinishedSemaphore[0]};

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.waitSemaphoreCount =
      static_cast<uint32_t>(graphicsSemaphores.size());
  submitInfo.pWaitSemaphores = graphicsSemaphores.data();
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      &this->pEngineCore->sync.renderFinishedSemaphore[0];
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Submit command buffers to the graphics queue
  validate_vk_result(vkQueueSubmit(this->pEngineCore->queue.graphics, 1,
                                   &submitInfo,
                                   this->pEngineCore->sync.drawFences[0]));

  // Prepare present info for swapchain presentation
  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores =
      &this->pEngineCore->sync.renderFinishedSemaphore[0];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores =
      &this->pEngineCore->sync.presentFinishedSemaphore[0];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &this->pEngineCore->swapchainData.swapchainKHR;
  presentInfo.pImageIndices = &imageIndex;

  // Present the rendered image to the screen
  VkResult presentImageResult =
      vkQueuePresentKHR(this->pEngineCore->queue.graphics, &presentInfo);

  // -- -- -- -- END LOADING SCREEN  -- -- -- -- //
}

gtp::LoadingScreen::LoadingScreen() {}

gtp::LoadingScreen::LoadingScreen(EngineCore *engineCorePtr) {
  // call init function
  this->InitLoadingScreen(engineCorePtr);
}

void gtp::LoadingScreen::Draw(CoreUI uiPtr) { this->DrawLoadingScreen(uiPtr); }
