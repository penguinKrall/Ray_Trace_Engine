#pragma once

#include <Commands.hpp>
#include <CoreExtensions.hpp>
// #include <Utilities_EngCore.hpp>

//--class to handle devices and components
class CoreDevice : public Commands {
public:
  CoreExtensions *coreExtensions = new CoreExtensions();

  //--ctor
  CoreDevice();

  // -- devices
  struct Devices {
    VkPhysicalDevice physical;
    VkDevice logical;
  };

  //--device capabilities data
  struct DeviceData {
    uint32_t selectedGPU = 0;
    std::vector<VkPhysicalDevice> deviceList;
    VkPhysicalDeviceFeatures features;
    // pNext structure for passing extension structures to device creation
    // void* deviceCreatepNextChain = nullptr;
    // raytracing properties and features
    VkPhysicalDeviceFeatures2 features2{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR
        accelerationStructureFeatures{};
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{};
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT
        physicalDeviceDescriptorIndexingFeatures{};
    VkPhysicalDeviceRayTracingPositionFetchFeaturesKHR
        enabledRayTracingPositionFetchFeatures{};
  };

  struct DeviceProperties {
    VkPhysicalDeviceProperties physicalDevice;
    VkPhysicalDeviceProperties2 physicalDevice2{};
    VkPhysicalDeviceMemoryProperties memory;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineKHR{};
    VkPhysicalDeviceAccelerationStructurePropertiesKHR
        accelerationStructureKHR{};
  };

  // member struct
  Devices devices{};
  DeviceData deviceData{};
  // DeviceExtensions deviceExtensions{};
  DeviceProperties deviceProperties{};

  // rt class
  // CoreExtensions coreExtensions;

  // -- GET PHYSICAL DEVICE
  VkResult getPhysicalDevice(VkInstance instance);

  //return logical device
  VkDevice GetLogicalDevice();

  bool checkExtensionSupport(const char *extensionName) const;
  uint32_t getMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties,
                         VkBool32 *memTypeFound = nullptr) const;
  VkDevice createLogicalDevice(VkSurfaceKHR surface);

  VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats,
                                 VkImageTiling tiling,
                                 VkFormatFeatureFlags featureFlags);

  void destroyDevice();
};
