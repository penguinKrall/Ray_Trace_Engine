#pragma once

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <string>
#include <array>
#include <assert.h>

namespace gtp {

	// -- buffer helper class
	class Buffer {

	public:
		//member datas
		//VkDevice device;
		struct BufferData {
			VkBuffer buffer = VK_NULL_HANDLE;
			VkDeviceMemory memory = VK_NULL_HANDLE;
			VkDescriptorBufferInfo descriptor;
			VkDeviceSize size = 0;
			VkDeviceSize alignment = 0;
			void* mapped = nullptr;
			VkBufferUsageFlags usageFlags;
			VkMemoryPropertyFlags memoryPropertyFlags;
			std::string bufferName = "unnamed";
			std::string bufferMemoryName = "unnamed";
			VkDeviceOrHostAddressConstKHR bufferDeviceAddress{};
		};

		//struct
		BufferData bufferData{};

		//ctor
		Buffer();

		//map
		VkResult map(VkDevice logicalDevice, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void unmap(VkDevice logicalDevice);

		//bind
		VkResult bind(VkDevice logicalDevice, VkDeviceSize offset = 0);

		void setupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void copyTo(void* data, VkDeviceSize size);

		VkResult flush(VkDevice logicalDevice, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		VkResult invalidate(VkDevice logicalDevice, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

		void destroy(VkDevice logicalDevice);

		VkBuffer CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
			VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, std::string bufferName);

		VkDeviceMemory AllocateBufferMemory(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, std::string bufferMemoryName);

		VkResult BindBufferMemory(VkDevice logicalDevice, void* data);

		static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes,
			VkMemoryPropertyFlags properties);

	};

}

