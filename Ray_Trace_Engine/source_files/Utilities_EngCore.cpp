#include "Utilities_EngCore.hpp"

//gtp::Utilities_EngCore::ImageData gtp::Utilities_EngCore::CreateImage(const std::string& fileName) {
//  return gtp::Utilities_EngCore::ImageData();
//}

std::string gtp::Utilities_EngCore::BuildPath(const std::string &fileName) {
  std::string path = std::filesystem::current_path().string() +"/"+ fileName;
  return path;
}

gtp::Utilities_EngCore::QueueFamilyIndices
gtp::Utilities_EngCore::getQueueFamilyIndices(VkPhysicalDevice physicalDevice,
                                              VkSurfaceKHR surface) {

  // get queue family count
  gtp::Utilities_EngCore::QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);
  // ouput count
  // std::cout << "\nqueue family count: " << queueFamilyCount << std::endl;

  // get queue family properties and fill list
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilyProperties.data());
  // output properties of each "count"
  // for (int i = 0; i < queueFamilyCount; i++) {
  //	std::cout << "\nCount[" << i << "]\tQueueFlags: " <<
  // queueFamilyProperties[i].queueFlags << std::endl;
  // }

  // count
  int idx = 0;

  // iterate through queue family list and get index for each queue
  for (const auto &queueFamily : queueFamilyProperties) {
    // check presentation support
    VkBool32 presentationSupport = false;
    if (vkGetPhysicalDeviceSurfaceSupportKHR(
            physicalDevice, idx, surface, &presentationSupport) != VK_SUCCESS) {
      throw std::invalid_argument(
          "Device does not support presentation queue!");
    }
    // graphics
    if (queueFamily.queueCount > 0 &&
        (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
        (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
      indices.graphics = idx;
      indices.compute = idx;
    }
    // compute
    // if (queueFamily.queueCount > 0 && (queueFamily.queueFlags &
    // VK_QUEUE_COMPUTE_BIT)) { 	indices.compute = idx;
    // }
    // presentation
    if (queueFamily.queueCount > 0 && presentationSupport) {
      indices.present = idx;
    }
    // transfer
    if (queueFamily.queueCount > 0 &&
        (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT)) {
      indices.transfer = idx;
    }
    // if all queues are assigned, break
    if (validateQueueFamilyIndices(indices)) {
      break;
    }

    // inc. count
    idx++;
  }

  return indices;
}

bool gtp::Utilities_EngCore::validateQueueFamilyIndices(
    QueueFamilyIndices queueFamily) {
  // see if all queues have been assigned
  if (queueFamily.graphics >= 0 && queueFamily.compute >= 0 &&
      queueFamily.present >= 0 && queueFamily.transfer >= 0) {
    return true;
  }

  else {
    return false;
  }
}

// -- ouput surface capabilities
void gtp::Utilities_EngCore::outputSurfaceCapabilities(
    const VkSurfaceCapabilitiesKHR &capabilities) {
  std::cout << "Surface Capabilities:" << std::endl;
  std::cout << "Min Image Count: " << capabilities.minImageCount << std::endl;
  std::cout << "Max Image Count: " << capabilities.maxImageCount << std::endl;
  std::cout << "Min Image Extent: " << capabilities.minImageExtent.width << "x"
            << capabilities.minImageExtent.height << std::endl;
  std::cout << "Max Image Extent: " << capabilities.maxImageExtent.width << "x"
            << capabilities.maxImageExtent.height << std::endl;
  std::cout << "Max Image Array Layers: " << capabilities.maxImageArrayLayers
            << std::endl;
  std::cout << "Supported Transform Flags: " << capabilities.supportedTransforms
            << std::endl;
  std::cout << "Current Transform: " << capabilities.currentTransform
            << std::endl;
  std::cout << "Supported Composite Alpha Flags: "
            << capabilities.supportedCompositeAlpha << std::endl;
  std::cout << "Supported Usage Flags: " << capabilities.supportedUsageFlags
            << std::endl;
}

std::string
gtp::Utilities_EngCore::physicalDeviceTypeString(VkPhysicalDeviceType type) {
  switch (type) {
#define STR(r)                                                                 \
  case VK_PHYSICAL_DEVICE_TYPE_##r:                                            \
    return #r
    STR(OTHER);
    STR(INTEGRATED_GPU);
    STR(DISCRETE_GPU);
    STR(VIRTUAL_GPU);
    STR(CPU);
#undef STR
  default:
    return "UNKNOWN_DEVICE_TYPE";
  }
}

bool gtp::Utilities_EngCore::loadFunctionPointer(
    PFN_vkVoidFunction &functionPointer, VkDevice logicalDevice,
    const char *functionName) {
  functionPointer = reinterpret_cast<PFN_vkVoidFunction>(
      vkGetDeviceProcAddr(logicalDevice, functionName));
  if (functionPointer) {
    std::cout << " Function: " << functionName << " loaded!" << std::endl;
    return true;
  } else {
    std::cout << " Failed to load function: " << functionName << std::endl;
    return false;
  }
}

bool gtp::Utilities_EngCore::loadFunctionPointer(
    PFN_vkVoidFunction &functionPointer, VkInstance instance,
    const char *functionName) {
  functionPointer = reinterpret_cast<PFN_vkVoidFunction>(
      vkGetInstanceProcAddr(instance, functionName));
  if (functionPointer) {
    std::cout << " function: " << functionName << " loaded!" << std::endl;
    return true;
  } else {
    std::cout << " Failed to load function: " << functionName << std::endl;
    return false;
  }
}

void gtp::Utilities_EngCore::setImageLayout(
    VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
    VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask) {
  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (oldImageLayout) {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    // Image layout is undefined (or does not matter)
    // Only valid as initial layout
    // No flags required, listed only for completeness
    imageMemoryBarrier.srcAccessMask = 0;
    break;

  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    // Image is preinitialized
    // Only valid as initial layout for linear images, preserves memory contents
    // Make sure host writes have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image is a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image is a depth/stencil attachment
    // Make sure any writes to the depth/stencil buffer have been finished
    imageMemoryBarrier.srcAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image is a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image is a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image is read by a shader
    // Make sure any shader reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
    // Image is a present source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;

  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (newImageLayout) {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    // Image will be used as a transfer destination
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    // Image will be used as a transfer source
    // Make sure any reads from the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    // Image will be used as a color attachment
    // Make sure any writes to the color buffer have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    // Image layout will be used as a depth/stencil attachment
    // Make sure any writes to depth/stencil buffer have been finished
    imageMemoryBarrier.dstAccessMask |=
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;

  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    // Image will be read in a shader (sampler, input attachment)
    // Make sure any writes to the image have been finished
    if (imageMemoryBarrier.srcAccessMask == 0) {
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;

  case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
    // Image is a present source
    // Make sure any writes to the image have been finished
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    break;

  default:
    // Other source layouts aren't handled (yet)
    break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0,
                       nullptr, 1, &imageMemoryBarrier);
}

uint32_t gtp::Utilities_EngCore::alignedSize(uint32_t value,
                                             uint32_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

void gtp::Utilities_EngCore::FlushCommandBuffer(VkDevice logicalDevice,
                                                VkCommandBuffer commandBuffer,
                                                VkQueue queue,
                                                VkCommandPool pool, bool free) {
  if (commandBuffer == VK_NULL_HANDLE) {
    return;
  }

  validate_vk_result(vkEndCommandBuffer(commandBuffer));

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  // Create fence to ensure that the command buffer has finished executing
  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = VK_FLAGS_NONE;

  VkFence fence;
  validate_vk_result(
      vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence));

  // Submit to the queue
  validate_vk_result(vkQueueSubmit(queue, 1, &submitInfo, fence));

  // Wait for the fence to signal that command buffer has finished executing
  validate_vk_result(vkWaitForFences(logicalDevice, 1, &fence, VK_TRUE,
                                     std::numeric_limits<uint64_t>::max()));

  // destroy fence
  vkDestroyFence(logicalDevice, fence, nullptr);
  if (free) {
    vkFreeCommandBuffers(logicalDevice, pool, 1, &commandBuffer);
  }
}

size_t alignedSize(size_t value, size_t alignment) {
  return (value + alignment - 1) & ~(alignment - 1);
}

VkDescriptorSetLayoutBinding
gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
    uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount,
    VkShaderStageFlags stageFlags, const VkSampler *pImmutableSamplers) {
  // declare and initialize struct
  VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{};
  descriptorSetLayoutBinding.binding = binding;
  descriptorSetLayoutBinding.descriptorType = descriptorType;
  descriptorSetLayoutBinding.descriptorCount = descriptorCount;
  descriptorSetLayoutBinding.stageFlags = stageFlags;
  descriptorSetLayoutBinding.pImmutableSamplers = pImmutableSamplers;
  // return initialized struct
  return descriptorSetLayoutBinding;
}
