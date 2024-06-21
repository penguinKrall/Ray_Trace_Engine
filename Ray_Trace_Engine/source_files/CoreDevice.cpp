#include "CoreDevice.hpp"

CoreDevice::CoreDevice() {}

uint32_t CoreDevice::getMemoryType(uint32_t typeBits,
                                   VkMemoryPropertyFlags properties,
                                   VkBool32 *memTypeFound) const {
  for (uint32_t i = 0; i < deviceProperties.memory.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((deviceProperties.memory.memoryTypes[i].propertyFlags & properties) ==
          properties) {
        if (memTypeFound) {
          *memTypeFound = true;
        }
        return i;
      }
    }
    typeBits >>= 1;
  }

  if (memTypeFound) {
    *memTypeFound = false;
    return 0;
  } else {
    throw std::invalid_argument("Could not find a matching memory type");
  }
}

// -- get physical device
VkResult CoreDevice::getPhysicalDevice(VkInstance instance) {

  // get device count
  uint32_t deviceCount = 0;

  if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) !=
      VK_SUCCESS) {
    throw std::invalid_argument(
        "Can't find GPUs that support Vulkan Instance!");
  } else {
    std::cout << "Physical Device Count: " << deviceCount << std::endl;
  }

  // add GPUs found to device list
  deviceData.deviceList.resize(deviceCount);
  if (vkEnumeratePhysicalDevices(instance, &deviceCount,
                                 deviceData.deviceList.data())) {
    throw std::invalid_argument("Failed to create GPU list!");
  }

  // output available GPUs
  std::cout << std::endl << "Available Physical Devices:" << "\n" << std::endl;
  for (uint32_t i = 0; i < deviceCount; i++) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(deviceData.deviceList[i], &deviceProperties);
    std::cout << "Device [" << i << "] : " << deviceProperties.deviceName
              << std::endl;
    std::cout << " Type: "
              << gtp::Utilities_EngCore::physicalDeviceTypeString(
                     deviceProperties.deviceType)
              << "\n";
    std::cout << " API: " << (deviceProperties.apiVersion >> 22) << "."
              << ((deviceProperties.apiVersion >> 12) & 0x3ff) << "."
              << (deviceProperties.apiVersion & 0xfff) << "\n"
              << std::endl;
  }

  // select physical device to be used for the Vulkan example
  deviceData.selectedGPU = 0;

  // assign selected GPU to physicalDevice
  devices.physical = deviceData.deviceList[deviceData.selectedGPU];

  // get properties, features, memory properties
  vkGetPhysicalDeviceProperties(devices.physical,
                                &deviceProperties.physicalDevice);
  vkGetPhysicalDeviceFeatures(devices.physical, &deviceData.features);
  vkGetPhysicalDeviceMemoryProperties(devices.physical,
                                      &deviceProperties.memory);

  // output device info
  std::cout << std::endl << "Selected Physical Device" << std::endl;
  std::cout << "Device Name: " << deviceProperties.physicalDevice.deviceName
            << std::endl;
  std::cout << "Device Type: "
            << gtp::Utilities_EngCore::physicalDeviceTypeString(
                   deviceProperties.physicalDevice.deviceType)
            << std::endl;
  std::cout << "API Version: "
            << (deviceProperties.physicalDevice.apiVersion >> 22) << "."
            << ((deviceProperties.physicalDevice.apiVersion >> 12) & 0x3ff)
            << "." << (deviceProperties.physicalDevice.apiVersion & 0xfff)
            << std::endl;

  // usage of selected memory properties
  std::cout << "Memory Types: " << deviceProperties.memory.memoryTypeCount
            << std::endl;
  std::cout << "Memory Heaps: " << deviceProperties.memory.memoryHeapCount
            << std::endl;

  for (int i = 0; i < static_cast<int>(deviceProperties.memory.memoryTypeCount);
       i++) {
    std::cout << "Memory Type [" << i << "]" << "\nMemory property flags: "
              << deviceProperties.memory.memoryTypes[i].propertyFlags
              << std::endl;
  }

  // get extension properties from device
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(devices.physical, nullptr,
                                       &extensionCount, nullptr);

  if (extensionCount != 0) {
    coreExtensions->deviceExtensions.properties.resize(extensionCount);
    vkEnumerateDeviceExtensionProperties(
        devices.physical, nullptr, &extensionCount,
        coreExtensions->deviceExtensions.properties.data());

    // output extension data
    std::cout << std::endl
              << "Physical Device Extension Properties:" << std::endl;
    for (const auto &extension : coreExtensions->deviceExtensions.properties) {
      coreExtensions->deviceExtensions.supported.emplace_back(
          extension.extensionName);
      // std::cout << "  Extension Name: " << extension.extensionName <<
      // std::endl; std::cout << "  Spec Version: " << extension.specVersion <<
      // std::endl;
    }

    // std::cout << std::endl << "supported device extensions: " << std::endl;
    // int supDevIDX = 0;
    // for (const auto& str : coreExtensions->deviceExtensions.supported) {
    //	std::cout << " [" << supDevIDX << "]" << str.c_str() << std::endl;
    //	++supDevIDX;
    // }

    // convert std::string to char*
    coreExtensions->deviceExtensions.enabled.reserve(
        coreExtensions->deviceExtensions.supported
            .size()); // Reserve space for efficiency

    // fill enabled extensions
    int skippedExtCount = 0;

    // skip this extension bc validation layers
    for (auto &str : coreExtensions->deviceExtensions.supported) {

      if (!(strcmp(str.data(), "VK_NV_cuda_kernel_launch") == 0) &&
          !(strcmp(str.data(), VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) ==
            0) &&
          !(strcmp(str.data(), "VK_LUNARG_direct_driver_loading") == 0)) {
        coreExtensions->deviceExtensions.enabled.push_back(str.data());
      }

      else {
        ++skippedExtCount;
      }
    }

    // -- output physical device enabled extension count
    std::cout << std::endl
              << "Physical Device Enabled Device Extension Count: "
              << coreExtensions->deviceExtensions.supported.size() -
                     skippedExtCount
              << std::endl;

    // for (int i = 0; i < coreExtensions->deviceExtensions.enabled.size(); i++)
    // { std::cout << " [" << i << "]" <<
    // coreExtensions->deviceExtensions.enabled[i] << std::endl;
    // }

  }

  else {
    std::cout << "No device extensions found." << std::endl;
  }

  // enable features required for ray tracing using feature chaining via pNext
  deviceData.bufferDeviceAddressFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
  deviceData.bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
  deviceData.bufferDeviceAddressFeatures.pNext = nullptr;

  deviceData.rayTracingPipelineFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
  deviceData.rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
  deviceData.rayTracingPipelineFeatures.pNext =
      &deviceData.bufferDeviceAddressFeatures;

  deviceData.accelerationStructureFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
  deviceData.accelerationStructureFeatures.accelerationStructure = VK_TRUE;
  deviceData.accelerationStructureFeatures.pNext =
      &deviceData.rayTracingPipelineFeatures;

  //// VK_KHR_ray_tracing_position_fetch has a new feature struct
  // deviceData.enabledRayTracingPositionFetchFeatures.sType =
  // VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_POSITION_FETCH_FEATURES_KHR;
  // deviceData.enabledRayTracingPositionFetchFeatures.rayTracingPositionFetch =
  // VK_TRUE; deviceData.enabledRayTracingPositionFetchFeatures.pNext =
  // &deviceData.accelerationStructureFeatures;

  deviceData.physicalDeviceDescriptorIndexingFeatures.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;

  deviceData.physicalDeviceDescriptorIndexingFeatures
      .shaderSampledImageArrayNonUniformIndexing = VK_TRUE;

  deviceData.physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray =
      VK_TRUE;

  deviceData.physicalDeviceDescriptorIndexingFeatures
      .descriptorBindingVariableDescriptorCount = VK_TRUE;

  deviceData.physicalDeviceDescriptorIndexingFeatures.pNext =
      &deviceData.accelerationStructureFeatures;

  deviceData.features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  deviceData.features2.pNext =
      &deviceData.physicalDeviceDescriptorIndexingFeatures;

  // vkGetPhysicalDeviceFeatures2(devices.physical, &deviceData.features2);

  // deviceData.deviceCreatepNextChain = &deviceData.features2;

  // Output enabled features
  std::cout << std::endl << "Enabled Features:" << std::endl;
  std::cout << "  Buffer Device Address: "
            << (deviceData.bufferDeviceAddressFeatures.bufferDeviceAddress
                    ? "Enabled"
                    : "Disabled")
            << std::endl;

  std::cout << "  Ray Tracing Pipeline: "
            << (deviceData.rayTracingPipelineFeatures.rayTracingPipeline
                    ? "Enabled"
                    : "Disabled")
            << std::endl;

  std::cout << "  Acceleration Structure: "
            << (deviceData.accelerationStructureFeatures.accelerationStructure
                    ? "Enabled"
                    : "Disabled")
            << std::endl;

  bool bufferDeviceAddressSupported = false;
  bool rayTracingSupported = false;
  bool accelerationStructureSupported = false;

  for (const auto &extension : coreExtensions->deviceExtensions.properties) {
    if (strcmp(extension.extensionName,
               VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0) {
      bufferDeviceAddressSupported = true;
    }
    if (strcmp(extension.extensionName,
               VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0) {
      rayTracingSupported = true;
    }
    if (strcmp(extension.extensionName,
               VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0) {
      accelerationStructureSupported = true;
    }
  }

  if (!rayTracingSupported || !accelerationStructureSupported ||
      !bufferDeviceAddressSupported) {
    throw std::invalid_argument("Required extensions not supported!");
  }

  if (!deviceData.bufferDeviceAddressFeatures.bufferDeviceAddress ||
      !deviceData.rayTracingPipelineFeatures.rayTracingPipeline ||
      !deviceData.accelerationStructureFeatures.accelerationStructure) {
    throw std::runtime_error("Failed to enable required features!");
  }

  // Output acceleration structure features
  std::cout << std::endl << "Supported Features:" << std::endl;
  std::cout << "  Buffer Device Address: "
            << (deviceData.bufferDeviceAddressFeatures.bufferDeviceAddress
                    ? "Supported"
                    : "Not Supported")
            << std::endl;
  std::cout << "  Raytracing Pipeline: "
            << (deviceData.rayTracingPipelineFeatures.rayTracingPipeline
                    ? "Supported"
                    : "Not Supported")
            << std::endl;
  std::cout << "  Acceleration Structure: "
            << (deviceData.accelerationStructureFeatures.accelerationStructure
                    ? "Supported"
                    : "Not Supported")
            << std::endl;

  // properties
  // rt pipeline
  deviceProperties.rayTracingPipelineKHR.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

  // acceleration structure
  deviceProperties.accelerationStructureKHR.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

  // chain pNext from deviceProperties.rayTracingPipelineKHR
  deviceProperties.accelerationStructureKHR.pNext =
      &deviceProperties.rayTracingPipelineKHR;

  // get deviceProperties.physicalDevice2
  deviceProperties.physicalDevice2.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
  deviceProperties.physicalDevice2.pNext =
      &deviceProperties.accelerationStructureKHR;
  vkGetPhysicalDeviceProperties2(devices.physical,
                                 &deviceProperties.physicalDevice2);

  // Output ray tracing pipeline properties
  std::cout << std::endl << "Ray Tracing Pipeline Properties:" << std::endl;
  std::cout << "  Max Recursion Depth: "
            << deviceProperties.rayTracingPipelineKHR.maxRayRecursionDepth
            << std::endl;
  std::cout << "  Max Ray Hit Attribute Size: "
            << deviceProperties.rayTracingPipelineKHR.maxRayHitAttributeSize
            << std::endl;
  // Add more properties as needed

  // Output acceleration structure properties
  std::cout << std::endl << "Acceleration Structure Properties:" << std::endl;
  std::cout << "  Max Instance Count: "
            << deviceProperties.accelerationStructureKHR.maxInstanceCount
            << std::endl;
  std::cout << "  Max Geometry Count: "
            << deviceProperties.accelerationStructureKHR.maxGeometryCount
            << std::endl;
  std::cout << "  Max Descriptor Set Acceleration Structures: "
            << deviceProperties.accelerationStructureKHR
                   .maxDescriptorSetAccelerationStructures
            << std::endl;
  // Add more properties as needed

  return VK_SUCCESS;
}

// -- get queue family indices
// gtp::Utilities_EngCore::QueueFamilyIndices
// CoreDevice::getQueueFamilyIndices(VkSurfaceKHR surface) {
//
//	//get queue family count
//	gtp::Utilities_EngCore::QueueFamilyIndices indices;
//	uint32_t queueFamilyCount = 0;
//	vkGetPhysicalDeviceQueueFamilyProperties(devices.physical,
//&queueFamilyCount, nullptr);
//
//	//get queue family properties and fill list
//	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
//	vkGetPhysicalDeviceQueueFamilyProperties(devices.physical,
//&queueFamilyCount, queueFamilyList.data());
//
//	//count
//	int idx = 0;
//
//	//iterate through queue family list and get index for each queue
//	for (const auto& queueFamily : queueFamilyList) {
//		//check presentation support
//		VkBool32 presentationSupport = false;
//		if (vkGetPhysicalDeviceSurfaceSupportKHR(devices.physical, idx,
// surface, &presentationSupport) != VK_SUCCESS) { 			throw
// std::invalid_argument("Device does not support presentation queue!");
//		}
//		//graphics
//		if (queueFamily.queueCount > 0 && queueFamily.queueFlags &
// VK_QUEUE_GRAPHICS_BIT) { 			indices.graphics = idx;
//		}
//		//compute
//		if (queueFamily.queueCount > 0 && queueFamily.queueFlags &
// VK_QUEUE_COMPUTE_BIT) { 			indices.compute = idx;
//		}
//		//presentation
//		if (queueFamily.queueCount > 0 && presentationSupport) {
//			indices.present = idx;
//		}
//		//transfer
//		if (queueFamily.queueCount > 0 && queueFamily.queueFlags &
// VK_QUEUE_TRANSFER_BIT) { 			indices.transfer = idx;
//		}
//		//if all queues are assigned, break
//		if (validateQueueFamilyIndices()) {
//			break;
//		}
//
//		//inc. count
//		idx++;
//
//	}
//
//	return indices;
//
//}

// -- validate queue family indices
// bool CoreDevice::validateQueueFamilyIndices() {
//
//	//see if all queues have been assigned
//	if (queue.queueFamilyIndices.graphics >= 0 &&
//		queue.queueFamilyIndices.compute >= 0 &&
//		queue.queueFamilyIndices.present >= 0 &&
//		queue.queueFamilyIndices.transfer >= 0) {
//		return true;
//	}
//
//	else {
//		return false;
//	}
//
//}

// -- check device extension support
bool CoreDevice::checkExtensionSupport(const char *extensionName) const {
  for (const auto &extension : coreExtensions->deviceExtensions.properties) {
    if (strcmp(extension.extensionName, extensionName) == 0) {
      return true;
    }
  }
  return false;
}

// -- create logical device
VkDevice CoreDevice::createLogicalDevice(VkSurfaceKHR surface) {

  // queue create infos
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};

  // default priority
  const float defaultQueuePriority(1.0f);

  queue.queueFamilyIndices =
      gtp::Utilities_EngCore::getQueueFamilyIndices(devices.physical, surface);

  std::cout << "Queue Family Indices " << std::endl;
  std::cout << " queueFamilyIndices.graphics: "
            << queue.queueFamilyIndices.graphics << std::endl;
  std::cout << " queueFamilyIndices.compute: "
            << queue.queueFamilyIndices.compute << std::endl;
  std::cout << " queueFamilyIndices.present: "
            << queue.queueFamilyIndices.present << std::endl;
  std::cout << " queueFamilyIndices.transfer: "
            << queue.queueFamilyIndices.transfer << std::endl;

  // Vector for Queue creation information, and set for family indices
  std::set<int> indices = {
      queue.queueFamilyIndices.graphics, queue.queueFamilyIndices.compute,
      queue.queueFamilyIndices.present, queue.queueFamilyIndices.transfer};

  // Queue that logical device needs to create and info to create it
  for (int queueFamilyIndex : indices) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  // enable device features
  // deviceData.features.samplerAnisotropy = VK_TRUE;
  // deviceData.features.sampleRateShading = VK_TRUE;
  // deviceData.features.shaderStorageImageMultisample = VK_TRUE;
  deviceData.features.shaderInt64 = VK_TRUE;

  // If a pNext(Chain) has been passed, we need to add it to the device creation
  // info
  // VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};
  // physicalDeviceFeatures2.sType =
  // VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  // physicalDeviceFeatures2.features = deviceData.features;
  // physicalDeviceFeatures2.pNext = deviceData.deviceCreatepNextChain;
  deviceData.features2.features = deviceData.features;
  vkGetPhysicalDeviceFeatures2(devices.physical, &deviceData.features2);

  // deviceData.deviceCreatepNextChain = &deviceData.features2;

  ////Information to create Logical Device (sometimes reffered to as "Device")
  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.pEnabledFeatures = nullptr;
  deviceCreateInfo.enabledExtensionCount =
      static_cast<uint32_t>(coreExtensions->deviceExtensions.enabled.size());
  deviceCreateInfo.ppEnabledExtensionNames =
      coreExtensions->deviceExtensions.enabled.data();
  deviceCreateInfo.pNext = &deviceData.features2;

  // Output enabled extensions
  std::cout << std::endl
            << "Logical Device Enabled Extension Count:"
            << coreExtensions->deviceExtensions.enabled.size() << std::endl;
  for (const auto &extension : coreExtensions->deviceExtensions.enabled) {
    std::cout << "  " << extension << std::endl;
  }

  if (vkCreateDevice(devices.physical, &deviceCreateInfo, nullptr,
                     &this->devices.logical) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create logical device");
  }

  // vkGetDeviceQueue(devices.logical, queue.queueFamilyIndices.graphics, 0,
  // &queue.graphics); vkGetDeviceQueue(devices.logical,
  // queue.queueFamilyIndices.compute, 0, &queue.compute);
  // vkGetDeviceQueue(devices.logical, queue.queueFamilyIndices.present, 0,
  // &queue.present); vkGetDeviceQueue(devices.logical,
  // queue.queueFamilyIndices.transfer, 0, &queue.transfer);

  return devices.logical;
}

VkFormat CoreDevice::chooseSupportedFormat(const std::vector<VkFormat> &formats,
                                           VkImageTiling tiling,
                                           VkFormatFeatureFlags featureFlags) {

  // loop through options and find suitable one
  for (VkFormat format : formats) {
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(devices.physical, format, &properties);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (properties.linearTilingFeatures & featureFlags) == featureFlags) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
               (properties.optimalTilingFeatures & featureFlags) ==
                   featureFlags) {
      return format;
    }
  }
  throw std::invalid_argument("Failed to find a matching Format!");
}

// -- destroy
void CoreDevice::destroyDevice() {
  // commands
  DestroyCommands(devices.logical);

  // logical device
  vkDestroyDevice(devices.logical, nullptr);
}
