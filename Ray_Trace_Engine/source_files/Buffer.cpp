#include "Buffer.hpp"

namespace gtp {

	Buffer::Buffer() {

	}

	// -- map memory range
	//@return VkResult
	VkResult Buffer::map(VkDevice logicalDevice, VkDeviceSize size, VkDeviceSize offset) {
		return vkMapMemory(logicalDevice, bufferData.memory, offset, size, 0, &bufferData.mapped);
	}

	// -- unmap memory range
	void Buffer::unmap(VkDevice logicalDevice) {
		if (bufferData.mapped) {
			vkUnmapMemory(logicalDevice, bufferData.memory);
			bufferData.mapped = nullptr;
		}
	}

	// -- bind allocated memory to buffer
	//@param offset (Optional) Byte offset (from the beginning) for the memory region to bind
	//@return VkResult of the bindBufferMemory call
	VkResult Buffer::bind(VkDevice logicalDevice, VkDeviceSize offset) {
		return vkBindBufferMemory(logicalDevice, bufferData.buffer, bufferData.memory, offset);
	}

	// -- setup descriptor 
	//@param size (Optional) Size of the memory range of the descriptor
	//@param offset (Optional) Byte offset from beginning
	void Buffer::setupDescriptor(VkDeviceSize size, VkDeviceSize offset) {
		bufferData.descriptor.offset = offset;
		bufferData.descriptor.buffer = bufferData.buffer;
		bufferData.descriptor.range = size;
	}

	// -- copy specified data to buffer
	//@param data Pointer to the data to copy
	//@param size Size of the data to copy in machine units
	void Buffer::copyTo(void* data, VkDeviceSize size) {
		assert(bufferData.mapped);
		memcpy(bufferData.mapped, data, size);
	}

	//Flush a memory range of the buffer to make it visible to the device
	//@note Only required for non-coherent memory
	//@param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
	//@param offset (Optional) Byte offset from beginning
	//@return VkResult of the flush call
	VkResult Buffer::flush(VkDevice logicalDevice, VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = bufferData.memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkFlushMappedMemoryRanges(logicalDevice, 1, &mappedRange);
	}

	//Invalidate a memory range of the buffer to make it visible to the host
	//@note Only required for non-coherent memory
	//@param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete buffer range.
	//@param offset (Optional) Byte offset from beginning
	//@return VkResult of the invalidate call
	VkResult Buffer::invalidate(VkDevice logicalDevice, VkDeviceSize size, VkDeviceSize offset) {
		VkMappedMemoryRange mappedRange = {};
		mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		mappedRange.memory = bufferData.memory;
		mappedRange.offset = offset;
		mappedRange.size = size;
		return vkInvalidateMappedMemoryRanges(logicalDevice, 1, &mappedRange);
	}


	//Release all Vulkan resources held by this buffer
	void Buffer::destroy(VkDevice logicalDevice) {
		if (bufferData.buffer) {
			vkDestroyBuffer(logicalDevice, bufferData.buffer, nullptr);
		}
		if (bufferData.memory) {
			vkFreeMemory(logicalDevice, bufferData.memory, nullptr);
		}
	}

	// -- find memory type index
	//@return 
	// if ((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags& properties) == properties) {
	//  return i }
	//
	uint32_t Buffer::findMemoryTypeIndex(
		VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties) {

		VkPhysicalDeviceMemoryProperties memoryProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if ((allowedTypes & (1 << i))
				&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		return 0;

	}

	// -- create buffer
	//	@return VkBuffer object
	VkBuffer Buffer::CreateBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
		VkBufferUsageFlags usageFlags, VkDeviceSize bufferSize, std::string bufferName) {

		//bufferData struct
		bufferData.usageFlags = usageFlags;
		bufferData.size = bufferSize;
		bufferData.bufferName = bufferName;

		//buffer create info
		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.usage = usageFlags;
		bufferCreateInfo.size = bufferSize;

		//create buffer
		if (vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &bufferData.buffer) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create buffer");
		}

		//return buffer
		return bufferData.buffer;
	}

	// -- allocate buffer memory
	//@return VkDeviceMemory object
	VkDeviceMemory Buffer::AllocateBufferMemory(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
		std::string bufferMemoryName) {

		//requirements
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(logicalDevice, bufferData.buffer, &memReqs);

		//bufferData struct
		bufferData.alignment = memReqs.alignment;
		bufferData.bufferMemoryName = bufferMemoryName;

		//memory allocate information
		VkMemoryAllocateInfo memAlloc{};
		memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		memAlloc.allocationSize = memReqs.size;
		memAlloc.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memReqs.memoryTypeBits, bufferData.memoryPropertyFlags);
		
		//enable this bs
		VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
		if (bufferData.usageFlags & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
			allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
			allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
			memAlloc.pNext = &allocFlagsInfo;
		}

		//allocate buffer memory
		if (vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &bufferData.memory) != VK_SUCCESS) {
			throw std::invalid_argument("failed to allocate buffer memory");
		}

		
		//return allocate buffer memory
		return bufferData.memory;
	}

	// -- bind buffer memory
	//@return VkResult
	VkResult Buffer::BindBufferMemory(VkDevice logicalDevice, void* data = nullptr) {
		// If a pointer to the buffer data has been passed, map the buffer and copy over the data
		if (data != nullptr) {
			if (map(logicalDevice) != VK_SUCCESS) {
				throw std::invalid_argument("failed to map buffer");
			}
			memcpy(bufferData.mapped, data, bufferData.size);
			if ((bufferData.memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
				flush(logicalDevice);

			unmap(logicalDevice);
		}

		// Initialize a default descriptor that covers the whole buffer size
		setupDescriptor();

		// Attach the memory to the buffer object
		return bind(logicalDevice);
	}
}