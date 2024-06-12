#include "CoreExtensions.hpp"

CoreExtensions::CoreExtensions() {

}

void CoreExtensions::loadFunctions(VkDevice logicalDevice) {

	std::cout << "\n" << "Load Raytracing Function Pointers" <<
		"\n'''''''''''''''''''''''''''''''''" <<  std::endl;

	vrt::Tools::loadFunctionPointer(
		reinterpret_cast<PFN_vkVoidFunction&>(this->vkGetBufferDeviceAddressKHR), logicalDevice,
		"vkGetBufferDeviceAddressKHR");

	vrt::Tools::loadFunctionPointer(
		reinterpret_cast<PFN_vkVoidFunction&>(this->vkCmdBuildAccelerationStructuresKHR), logicalDevice,
		"vkCmdBuildAccelerationStructuresKHR");
	vrt::Tools::loadFunctionPointer(
		reinterpret_cast<PFN_vkVoidFunction&>(this->vkBuildAccelerationStructuresKHR), logicalDevice,
		"vkBuildAccelerationStructuresKHR");
	vrt::Tools::loadFunctionPointer(
		reinterpret_cast<PFN_vkVoidFunction&>(this->vkCreateAccelerationStructureKHR), logicalDevice,
		"vkCreateAccelerationStructureKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkDestroyAccelerationStructureKHR), logicalDevice,
		"vkDestroyAccelerationStructureKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkGetAccelerationStructureBuildSizesKHR), logicalDevice,
		"vkGetAccelerationStructureBuildSizesKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkGetAccelerationStructureDeviceAddressKHR), logicalDevice,
		"vkGetAccelerationStructureDeviceAddressKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkCmdTraceRaysKHR), logicalDevice,
		"vkCmdTraceRaysKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkGetRayTracingShaderGroupHandlesKHR), logicalDevice,
		"vkGetRayTracingShaderGroupHandlesKHR");
	vrt::Tools::loadFunctionPointer(
reinterpret_cast<PFN_vkVoidFunction&>(this->vkCreateRayTracingPipelinesKHR), logicalDevice,
		"vkCreateRayTracingPipelinesKHR");
}
