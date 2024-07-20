#include "Utilities_Renderer.hpp"

void Utilities_Renderer::CreateColorStorageImage(
    EngineCore *pEngineCore, Utilities_Renderer::StorageImage *storageImage,
    std::string name) {
  // image create info
  VkImageCreateInfo imageCreateInfo{};
  imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.format =
      pEngineCore->swapchainData.assignedSwapchainImageFormat.format;
  imageCreateInfo.extent.width =
      pEngineCore->swapchainData.swapchainExtent2D.width;
  imageCreateInfo.extent.height =
      pEngineCore->swapchainData.swapchainExtent2D.height;
  imageCreateInfo.extent.depth = 1;
  imageCreateInfo.mipLevels = 1;
  imageCreateInfo.arrayLayers = 1;
  imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageCreateInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // create image
  pEngineCore->AddObject(
      [pEngineCore, &imageCreateInfo, &storageImage]() {
        return pEngineCore->objCreate.VKCreateImage(&imageCreateInfo, nullptr,
                                                    &storageImage->image);
      },
      name);

  // image memory requirements
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(pEngineCore->GetLogicalDevice(),
                               storageImage->image, &memReqs);

  // image memory allocation information
  VkMemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.allocationSize = memReqs.size;
  memoryAllocateInfo.memoryTypeIndex = pEngineCore->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  pEngineCore->AddObject(
      [pEngineCore, &memoryAllocateInfo, &storageImage]() {
        return pEngineCore->objCreate.VKAllocateMemory(
            &memoryAllocateInfo, nullptr, &storageImage->memory);
      },
      name + "_memory");

  // bind image memory
  if (vkBindImageMemory(pEngineCore->GetLogicalDevice(), storageImage->image,
                        storageImage->memory, 0) != VK_SUCCESS) {
    throw std::invalid_argument("failed to bind image memory");
  }

  // image view create info
  VkImageViewCreateInfo imageViewCreateInfo{};
  imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  imageViewCreateInfo.format =
      pEngineCore->swapchainData.assignedSwapchainImageFormat.format;
  imageViewCreateInfo.subresourceRange = {};
  imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
  imageViewCreateInfo.subresourceRange.levelCount = 1;
  imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
  imageViewCreateInfo.subresourceRange.layerCount = 1;
  imageViewCreateInfo.image = storageImage->image;

  // create image view and map name/handle for debug
  pEngineCore->AddObject(
      [pEngineCore, &imageViewCreateInfo, &storageImage]() {
        return pEngineCore->objCreate.VKCreateImageView(
            &imageViewCreateInfo, nullptr, &storageImage->view);
      },
      name + "view");

  // create temporary command buffer
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // transition image layout
  gtp::Utilities_EngCore::setImageLayout(
      cmdBuffer, storageImage->image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

  // submit temporary command buffer
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);
}

void Utilities_Renderer::StorageImage::Destroy(EngineCore *engineCorePtr) {
  if (this->view) {
    vkDestroyImageView(engineCorePtr->GetLogicalDevice(), this->view, nullptr);
  }

  if (this->image) {
    vkDestroyImage(engineCorePtr->GetLogicalDevice(), this->image, nullptr);
  }

  if (this->memory) {
    vkFreeMemory(engineCorePtr->GetLogicalDevice(), this->memory, nullptr);
  }
}

void Utilities_Renderer::PipelineData::Destroy(EngineCore *pEngineCore) {
  if (this->pipeline) {
    vkDestroyPipeline(pEngineCore->GetLogicalDevice(), this->pipeline, nullptr);
  }

  if (this->pipelineLayout) {
    vkDestroyPipelineLayout(pEngineCore->GetLogicalDevice(),
                            this->pipelineLayout, nullptr);
  }

  if (this->descriptorSetLayout) {
    vkDestroyDescriptorSetLayout(pEngineCore->GetLogicalDevice(),
                                 this->descriptorSetLayout, nullptr);
  }

  if (this->descriptorPool) {
    vkDestroyDescriptorPool(pEngineCore->GetLogicalDevice(),
                            this->descriptorPool, nullptr);
  }
}

void Utilities_Renderer::ShaderBindingTableData::Destroy(
    EngineCore *pEngineCore) {
  this->raygenShaderBindingTable.destroy(pEngineCore->GetLogicalDevice());
  this->hitShaderBindingTable.destroy(pEngineCore->GetLogicalDevice());
  this->missShaderBindingTable.destroy(pEngineCore->GetLogicalDevice());
}
