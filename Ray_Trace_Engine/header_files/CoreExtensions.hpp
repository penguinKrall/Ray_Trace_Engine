#pragma once
#include <vulkan/vulkan.h>
#include <iostream>
#include <Tools.hpp>

class CoreExtensions {
public:

	//CoreExtensions* pCoreExtensions = this;

	//extensions supported by the device
	struct DeviceExtensions {
		std::vector<std::string> supported;
		std::vector<const char*> enabled;
		//extension properties
		std::vector<VkExtensionProperties> properties{};

	};

	//function pointers
	//struct RaytracingExtensions {
		PFN_vkGetBufferDeviceAddressKHR vkGetBufferDeviceAddressKHR = nullptr;
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = nullptr;
		PFN_vkDestroyAccelerationStructureKHR vkDestroyAccelerationStructureKHR = nullptr;
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = nullptr;
		PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = nullptr;
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = nullptr;
		PFN_vkBuildAccelerationStructuresKHR vkBuildAccelerationStructuresKHR = nullptr;
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = nullptr;
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = nullptr;
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = nullptr;
	//};


	//--device extensions
	DeviceExtensions deviceExtensions{};
	//--raytracing extensions
	//RaytracingExtensions raytracingExtensions{};

	CoreExtensions();
	//bool loadFunctionPointer(PFN_vkVoidFunction& functionPointer, VkDevice logicalDevice, const char* functionName);
	void loadFunctions(VkDevice logicalDevice);
};

