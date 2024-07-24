#include "CoreExtensions.hpp"

CoreExtensions::CoreExtensions() {}

void CoreExtensions::loadFunctions(VkDevice logicalDevice) {

  std::cout << "\n"
            << "Load Raytracing Function Pointers"
            << "\n'''''''''''''''''''''''''''''''''" << std::endl;

  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(this->vkGetBufferDeviceAddressKHR),
      logicalDevice, "vkGetBufferDeviceAddressKHR");

  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkCmdBuildAccelerationStructuresKHR),
      logicalDevice, "vkCmdBuildAccelerationStructuresKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkBuildAccelerationStructuresKHR),
      logicalDevice, "vkBuildAccelerationStructuresKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkCreateAccelerationStructureKHR),
      logicalDevice, "vkCreateAccelerationStructureKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkDestroyAccelerationStructureKHR),
      logicalDevice, "vkDestroyAccelerationStructureKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkGetAccelerationStructureBuildSizesKHR),
      logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkGetAccelerationStructureDeviceAddressKHR),
      logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(this->vkCmdTraceRaysKHR),
      logicalDevice, "vkCmdTraceRaysKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkGetRayTracingShaderGroupHandlesKHR),
      logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");
  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          this->vkCreateRayTracingPipelinesKHR),
      logicalDevice, "vkCreateRayTracingPipelinesKHR");
}
