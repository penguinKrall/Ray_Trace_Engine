#include "ObjectMouseSelect.hpp"

void gtp::ObjectMouseSelect::InitObjectMouseSelect(EngineCore *engineCorePtr) {
  // init core pointer
  this->pEngineCore = engineCorePtr;

  // create color id image
  this->CreateColorIDStorageImage();

  // create color id image buffer
  this->CreateColorIDImageBuffer();
}

void gtp::ObjectMouseSelect::CreateColorIDImageBuffer() {
  // name color image id buffer
  buffers.colorIDImageBuffer.bufferData.bufferName =
      "ObjectMouseSelect_Image_ID_Buffer";
  buffers.colorIDImageBuffer.bufferData.bufferMemoryName =
      "ObjectMouseSelect_Image_ID_BufferMemory";

  // buffer size
  VkDeviceSize bufferSize =
      pEngineCore->swapchainData.swapchainExtent2D.width *
      pEngineCore->swapchainData.swapchainExtent2D.height * 4;

  // create color id image buffer
  validate_vk_result(pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &buffers.colorIDImageBuffer, bufferSize, nullptr));
}

void gtp::ObjectMouseSelect::CreateColorIDStorageImage() {
  // color id storage image
  Utilities_AS::createStorageImage(this->pEngineCore,
                                   &this->colorIDStorageImage,
                                   "ObjectMouseSelect_colorIDStorageImage");
}

gtp::ObjectMouseSelect::ObjectMouseSelect() {}

gtp::ObjectMouseSelect::ObjectMouseSelect(EngineCore *engineCorePtr) {
  this->InitObjectMouseSelect(engineCorePtr);

  // output succesful instance
  std::cout << "succesfully initialized object mouse select class."
            << std::endl;
}

Utilities_AS::StorageImage gtp::ObjectMouseSelect::GetIDImage() {
  return this->colorIDStorageImage;
}

gtp::Buffer gtp::ObjectMouseSelect::GetIDBuffer() {
  return this->buffers.colorIDImageBuffer;
}

void gtp::ObjectMouseSelect::HandleResize() {
  // destroy image and buffer
  // object id image
  this->colorIDStorageImage.Destroy(this->pEngineCore);

  // object id buffer
  this->buffers.colorIDImageBuffer.destroy(this->pEngineCore->devices.logical);

  // create image and buffer
  this->CreateColorIDStorageImage();
  this->CreateColorIDImageBuffer();
}

void gtp::ObjectMouseSelect::RetrieveObjectID() {
  if (this->pEngineCore->posX <
          this->pEngineCore->swapchainData.swapchainExtent2D.width &&
      this->pEngineCore->posX > 0) {
    if (this->pEngineCore->posY <
            this->pEngineCore->swapchainData.swapchainExtent2D.height &&
        this->pEngineCore->posY > 0) {
      VkCommandBuffer commandBuffer =
          pEngineCore->objCreate.VKCreateCommandBuffer(
              VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

      // subresource range
      VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0,
                                                  1, 0, 1};

      // set color ID image layout to transfer src optimal
      gtp::Utilities_EngCore::setImageLayout(
          commandBuffer, colorIDStorageImage.image, VK_IMAGE_LAYOUT_GENERAL,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

      // copy region
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;
      region.imageOffset = {0, 0, 0};
      region.imageExtent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                            pEngineCore->swapchainData.swapchainExtent2D.height,
                            1};

      vkCmdCopyImageToBuffer(commandBuffer, colorIDStorageImage.image,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             this->buffers.colorIDImageBuffer.bufferData.buffer,
                             1, &region);

      // Map the buffer memory
      void *data;
      vkMapMemory(pEngineCore->devices.logical,
                  this->buffers.colorIDImageBuffer.bufferData.memory, 0,
                  VK_WHOLE_SIZE, 0, &data);

      // Adjust mouse coordinates if necessary
      // auto adjustedY =
      //    static_cast<int>(pEngineCore->swapchainData.swapchainExtent2D.height
      //    -
      //                     1 - this->pEngineCore->posY);

      auto adjustedY = this->pEngineCore->posY;

      // Calculate the index
      int index = static_cast<int>(
          (adjustedY * pEngineCore->swapchainData.swapchainExtent2D.width +
           this->pEngineCore->posX) *
          4);

      // Retrieve the color at the mouse position
      uint8_t *pixel = static_cast<uint8_t *>(data) + index;
      uint8_t red = pixel[0];
      uint8_t green = pixel[1];
      uint8_t blue = pixel[2];
      uint8_t alpha = pixel[3];

      // Unmap the buffer memory
      vkUnmapMemory(pEngineCore->devices.logical,
                    this->buffers.colorIDImageBuffer.bufferData.memory);

      // Identify the object using the color
      float objectIDRed = red;
      float objectIDGreen = green;
      float objectIDBlue = blue;
      float objectIDAlpha = alpha;

      std::cout << "Selected Object ID: \n" << objectIDRed << std::endl;
      std::cout << "mouse position" << "\nx: " << this->pEngineCore->posX
                << "\ny: " << this->pEngineCore->posY << std::endl;
      std::cout << "adjustedY: " << adjustedY << "\nindex: " << index
                << std::endl;

      //<< objectIDGreen << std::endl
      //<< objectIDBlue << std::endl
      //<< objectIDAlpha << std::endl;

      // set color ID image layout to transfer src optimal
      gtp::Utilities_EngCore::setImageLayout(
          commandBuffer, colorIDStorageImage.image,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
          subresourceRange);

      // end and submit and destroy command buffer
      pEngineCore->FlushCommandBuffer(commandBuffer,
                                      pEngineCore->queue.graphics,
                                      pEngineCore->commandPools.graphics, true);
    }
  }
}

void gtp::ObjectMouseSelect::DestroyObjectMouseSelect() {
  this->buffers.colorIDImageBuffer.destroy(
      this->pEngineCore->GetLogicalDevice());
  this->colorIDStorageImage.Destroy(this->pEngineCore);
}
