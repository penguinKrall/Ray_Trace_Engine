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

  //output succesful instance
  std::cout << "succesfully initialized object mouse select class." << std::endl;

}

void gtp::ObjectMouseSelect::DestroyObjectMouseSelect() {
  this->buffers.colorIDImageBuffer.destroy(this->pEngineCore->GetLogicalDevice());
  this->colorIDStorageImage.Destroy(this->pEngineCore);
}
