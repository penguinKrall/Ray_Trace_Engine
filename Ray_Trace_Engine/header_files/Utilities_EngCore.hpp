#pragma once

//#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN

// Custom define for better code readability
#define VK_FLAGS_NONE 0
// Default fence timeout in nanoseconds
#define DEFAULT_FENCE_TIMEOUT 100000000000

#include <vector>
#include <array>
#include <string>

//#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <array>
#include <unordered_map>
#include <numeric>
#include <ctime>
#include <iostream>
#include <chrono>
#include <random>
#include <algorithm>
#include <set>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>

#include <Utilities_EnumStringHelper.hpp>
#include <Buffer.hpp>
#include <Camera.hpp>

#include <fstream>
#include <filesystem>

//const int scr_width = 1920;
//const int scr_height = 1024;
const int frame_draws = 3;

// -- vulkan ray tracing
//@brief i'm new to namespaces..
namespace gtp {

	// -- tools class -- 
	//@brief static functions used by all levels of program
	class Utilities_EngCore {

	public:

		//queue family indices
		struct QueueFamilyIndices {
			int graphics = -1;
			int compute = -1;
			int present = -1;
			int transfer = -1;
		};

		// -- get queue family indices
		static QueueFamilyIndices getQueueFamilyIndices(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

		// -- validate queue family indices
		static bool validateQueueFamilyIndices(QueueFamilyIndices queueFamily);

		// -- output surface capabilities
		//@brief output surface capabilities data to console for debugging
		static void outputSurfaceCapabilities(const VkSurfaceCapabilitiesKHR& capabilities);

		// -- physical device type to string
		//@brief output device data to console for debugging
		static std::string physicalDeviceTypeString(VkPhysicalDeviceType type);
		static bool loadFunctionPointer(PFN_vkVoidFunction& functionPointer, VkDevice logicalDevice, const char* functionName);
		static bool loadFunctionPointer(PFN_vkVoidFunction& functionPointer, VkInstance instance, const char* functionName);

		// -- set image layout
		//@brief create an image memory barrier for changing the layout of brief an image and put it into an active command buffer
		static void setImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		// -- aligned size(uint32_t)
		static uint32_t alignedSize(uint32_t value, uint32_t alignment);

		// -- aligned size(size_t)
		static size_t alignedSize(size_t value, size_t alignment);

		struct VkInitializers {
			// -- descriptor set layout binding 
			//@brief saves some space by returning an initialized struct of VkDescriptorSetLayoutBinding
			static VkDescriptorSetLayoutBinding descriptorSetLayoutBinding( uint32_t binding, VkDescriptorType descriptorType,
				uint32_t descriptorCount, VkShaderStageFlags stageFlags, const VkSampler* pImmutableSamplers);

		};

	};
}