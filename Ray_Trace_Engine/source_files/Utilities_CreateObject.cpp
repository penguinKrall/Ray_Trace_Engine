#include "Utilities_CreateObject.hpp"

Utilities_CreateObject::Utilities_CreateObject() {}

Utilities_CreateObject::Utilities_CreateObject(VkPhysicalDevice *physicalDevice,
                                               VkDevice *logicalDevice,
                                               CoreExtensions *coreExtensions,
                                               CoreDebug *coreDebug,
                                               VkCommandPool commandPool) {
  InitUtilities_CreateObject(physicalDevice, logicalDevice, coreExtensions,
                             coreDebug, commandPool);
}

void Utilities_CreateObject::InitUtilities_CreateObject(
    VkPhysicalDevice *physicalDevice, VkDevice *logicalDevice,
    CoreExtensions *coreExtensions, CoreDebug *coreDebug,
    VkCommandPool commandPool) {
  this->devices.physical = physicalDevice;
  this->devices.logical = logicalDevice;
  this->coreExtensions = coreExtensions;
  this->pCoreDebug = coreDebug;
  this->commandPool = commandPool;
}

VkBuffer
Utilities_CreateObject::VKCreateBuffer(VkBufferCreateInfo *bufferCreateInfo,
                                       VkAllocationCallbacks *allocator,
                                       VkBuffer *buffer) {
  validate_vk_result(
      vkCreateBuffer(*devices.logical, bufferCreateInfo, allocator, buffer));

  return *buffer;
}

VkDeviceMemory Utilities_CreateObject::VKAllocateMemory(
    VkMemoryAllocateInfo *memoryAllocateInfo, VkAllocationCallbacks *allocator,
    VkDeviceMemory *memory) {
  validate_vk_result(vkAllocateMemory(*devices.logical, memoryAllocateInfo,
                                      allocator, memory));

  return *memory;
}

VkCommandBuffer
Utilities_CreateObject::VKCreateCommandBuffer(VkCommandBufferLevel level,
                                              bool begin) {
  // allocate info
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
  commandBufferAllocateInfo.sType =
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = commandPool;
  commandBufferAllocateInfo.level = level;
  commandBufferAllocateInfo.commandBufferCount = 1;

  VkCommandBuffer cmdBuffer;
  validate_vk_result(vkAllocateCommandBuffers(
      *devices.logical, &commandBufferAllocateInfo, &cmdBuffer));

  // If requested, also start recording for the new command buffer
  if (begin) {
    VkCommandBufferBeginInfo cmdBufferBeginInfo{};
    cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    validate_vk_result(vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo));
  }

  return cmdBuffer;
}

VkImage
Utilities_CreateObject::VKCreateImage(VkImageCreateInfo *imageCreateInfo,
                                      VkAllocationCallbacks *allocator,
                                      VkImage *image) {
  validate_vk_result(
      vkCreateImage(*devices.logical, imageCreateInfo, allocator, image));

  return *image;
}

VkImageView
Utilities_CreateObject::VKCreateImageView(VkImageViewCreateInfo *createInfo,
                                          VkAllocationCallbacks *allocator,
                                          VkImageView *imageView) {
  validate_vk_result(
      vkCreateImageView(*devices.logical, createInfo, allocator, imageView));
  return *imageView;
}

VkSampler
Utilities_CreateObject::VKCreateSampler(VkSamplerCreateInfo *createInfo,
                                        VkAllocationCallbacks *allocator,
                                        VkSampler *sampler) {
  validate_vk_result(
      vkCreateSampler(*devices.logical, createInfo, allocator, sampler));
  return *sampler;
}

VkRenderPass
Utilities_CreateObject::VKCreateRenderPass(VkRenderPassCreateInfo *createInfo,
                                           VkAllocationCallbacks *allocator,
                                           VkRenderPass *renderPass) {
  validate_vk_result(
      vkCreateRenderPass(*devices.logical, createInfo, allocator, renderPass));
  return *renderPass;
}

VkPipelineLayout Utilities_CreateObject::VKCreatePipelineLayout(
    VkPipelineLayoutCreateInfo *createInfo, VkAllocationCallbacks *allocator,
    VkPipelineLayout *pipelineLayout) {
  validate_vk_result(vkCreatePipelineLayout(*devices.logical, createInfo,
                                            allocator, pipelineLayout));

  return *pipelineLayout;
}

VkPipeline Utilities_CreateObject::VKCreateRaytracingPipeline(
    VkRayTracingPipelineCreateInfoKHR *createInfo,
    VkAllocationCallbacks *allocator, VkPipeline *pipeline) {
  validate_vk_result(coreExtensions->vkCreateRayTracingPipelinesKHR(
      *devices.logical, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, createInfo,
      allocator, pipeline));
  return *pipeline;
}

VkDescriptorPool Utilities_CreateObject::VKCreateDescriptorPool(
    VkDescriptorPoolCreateInfo *descriptorPoolCreateInfo,
    VkAllocationCallbacks *allocator, VkDescriptorPool *descriptorPool) {
  validate_vk_result(vkCreateDescriptorPool(
      *devices.logical, descriptorPoolCreateInfo, allocator, descriptorPool));
  return *descriptorPool;
}

VkDescriptorSetLayout Utilities_CreateObject::VKCreateDescriptorSetLayout(
    VkDescriptorSetLayoutCreateInfo *createInfo,
    VkAllocationCallbacks *allocator,
    VkDescriptorSetLayout *descriptorSetLayout) {
  validate_vk_result(vkCreateDescriptorSetLayout(
      *devices.logical, createInfo, allocator, descriptorSetLayout));
  return *descriptorSetLayout;
}

VkDescriptorSet Utilities_CreateObject::VKAllocateDescriptorSet(
    VkDescriptorSetAllocateInfo *descriptorSetAllocateInfo,
    VkAllocationCallbacks *allocator, VkDescriptorSet *descriptorSet) {
  validate_vk_result(vkAllocateDescriptorSets(
      *devices.logical, descriptorSetAllocateInfo, descriptorSet));
  return *descriptorSet;
}

VkShaderModule Utilities_CreateObject::VKCreateShaderModule(
    VkShaderModuleCreateInfo *createInfo, VkAllocationCallbacks *allocator,
    VkShaderModule *shaderModule) {
  validate_vk_result(vkCreateShaderModule(*devices.logical, createInfo,
                                          allocator, shaderModule));
  return *shaderModule;
}

VkSemaphore Utilities_CreateObject::VKCreateSemaphore(
    VkSemaphoreCreateInfo *semaphoreCreateInfo,
    VkAllocationCallbacks *allocator, VkSemaphore *semaphore) {
  validate_vk_result(vkCreateSemaphore(*devices.logical, semaphoreCreateInfo,
                                       allocator, semaphore));
  return *semaphore;
}

VkFence
Utilities_CreateObject::VKCreateFence(VkFenceCreateInfo *fenceCreateInfo,
                                      VkAllocationCallbacks *allocator,
                                      VkFence *fence) {
  validate_vk_result(
      vkCreateFence(*devices.logical, fenceCreateInfo, allocator, fence));
  return *fence;
}

VkAccelerationStructureKHR
Utilities_CreateObject::VKCreateAccelerationStructureKHR(
    VkAccelerationStructureCreateInfoKHR *accelerationStructureCreateInfo,
    VkAllocationCallbacks *allocator,
    VkAccelerationStructureKHR *accelerationStructureKHR) {
  // function pointer in CoreExtensions.hpp -- class instance
  coreExtensions->vkCreateAccelerationStructureKHR(
      *devices.logical, accelerationStructureCreateInfo, allocator,
      accelerationStructureKHR);
  return *accelerationStructureKHR;
}
