#include "ObjCreate.hpp"

ObjCreate::ObjCreate() {
}

ObjCreate::ObjCreate(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice,
	CoreExtensions* coreExtensions, CoreDebug* coreDebug, VkCommandPool commandPool) {
	InitObjCreate(physicalDevice, logicalDevice, coreExtensions, coreDebug, commandPool);
}

void ObjCreate::InitObjCreate(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice,
	CoreExtensions* coreExtensions, CoreDebug* coreDebug, VkCommandPool commandPool) {
	this->devices.physical = physicalDevice;
	this->devices.logical = logicalDevice;
	this->coreExtensions = coreExtensions;
	this->pCoreDebug = coreDebug;
	this->commandPool = commandPool;
}

VkBuffer ObjCreate::VKCreateBuffer(VkBufferCreateInfo* bufferCreateInfo, VkAllocationCallbacks* allocator, VkBuffer* buffer) {
	if (vkCreateBuffer(*devices.logical, bufferCreateInfo, allocator, buffer) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create buffer");
	}
	return *buffer;
}

VkDeviceMemory ObjCreate::VKAllocateMemory(VkMemoryAllocateInfo* memoryAllocateInfo, VkAllocationCallbacks* allocator,
	VkDeviceMemory* memory) {
	if (vkAllocateMemory(*devices.logical, memoryAllocateInfo, allocator, memory)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to allocate memory");
	}

	return *memory;
}

VkCommandBuffer ObjCreate::VKCreateCommandBuffer(VkCommandBufferLevel level, bool begin) {
	//allocate info
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	if (vkAllocateCommandBuffers(*devices.logical, &commandBufferAllocateInfo, &cmdBuffer) != VK_SUCCESS) {
		throw std::invalid_argument("failed to allocate command buffer");
	}

	// If requested, also start recording for the new command buffer
	if (begin) {
		VkCommandBufferBeginInfo cmdBufferBeginInfo{};
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		if (vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo) != VK_SUCCESS) {
			throw std::invalid_argument("failed to start recording command buffer");
		}
	}

	return cmdBuffer;

}

VkImage ObjCreate::VKCreateImage(VkImageCreateInfo* imageCreateInfo, VkAllocationCallbacks* allocator, VkImage* image) {
	if (vkCreateImage(*devices.logical, imageCreateInfo, allocator, image) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create image");
	}
	return *image;
}

VkImageView ObjCreate::VKCreateImageView(VkImageViewCreateInfo* createInfo,
	VkAllocationCallbacks* allocator, VkImageView* imageView) {
	if (vkCreateImageView(*devices.logical, createInfo, allocator, imageView) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create image view");
	}
	return *imageView;
}

VkSampler ObjCreate::VKCreateSampler(VkSamplerCreateInfo* createInfo, VkAllocationCallbacks* allocator, VkSampler* sampler) {
	if (vkCreateSampler(*devices.logical, createInfo, allocator, sampler) != VK_SUCCESS) {
		throw std::invalid_argument(" failed to create sampler!");
	}
	return *sampler;
}

VkRenderPass ObjCreate::VKCreateRenderPass(VkRenderPassCreateInfo* createInfo, VkAllocationCallbacks* allocator, VkRenderPass* renderPass) {
	if (vkCreateRenderPass(*devices.logical, createInfo, allocator, renderPass) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create renderpass");
	}
	return *renderPass;
}

VkPipelineLayout ObjCreate::VKCreatePipelineLayout(VkPipelineLayoutCreateInfo* createInfo,
	VkAllocationCallbacks* allocator, VkPipelineLayout* pipelineLayout) {
	if (vkCreatePipelineLayout(*devices.logical, createInfo, allocator, pipelineLayout)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to create pipeline layout");
	}
	return *pipelineLayout;
}

VkPipeline ObjCreate::VKCreateRaytracingPipeline(VkRayTracingPipelineCreateInfoKHR* createInfo,
	VkAllocationCallbacks* allocator, VkPipeline* pipeline) {
	if (coreExtensions->vkCreateRayTracingPipelinesKHR(*devices.logical, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
		createInfo, allocator, pipeline) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create raytracing pipeline");
	}
	return *pipeline;
}

VkDescriptorPool ObjCreate::VKCreateDescriptorPool(VkDescriptorPoolCreateInfo* descriptorPoolCreateInfo,
	VkAllocationCallbacks* allocator, VkDescriptorPool* descriptorPool) {
	if (vkCreateDescriptorPool(*devices.logical, descriptorPoolCreateInfo, allocator, descriptorPool)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to create descriptor pool");
	}
	return *descriptorPool;
}

VkDescriptorSetLayout ObjCreate::VKCreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* createInfo,
	VkAllocationCallbacks* allocator, VkDescriptorSetLayout* descriptorSetLayout) {
	if (vkCreateDescriptorSetLayout(*devices.logical, createInfo, allocator, descriptorSetLayout) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create descriptor set layout");
	}
	return *descriptorSetLayout;
}

VkDescriptorSet ObjCreate::VKAllocateDescriptorSet(VkDescriptorSetAllocateInfo* descriptorSetAllocateInfo,
	VkAllocationCallbacks* allocator, VkDescriptorSet* descriptorSet) {
	if (vkAllocateDescriptorSets(*devices.logical, descriptorSetAllocateInfo, descriptorSet) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create descriptor set");
	}
	return *descriptorSet;
}

VkShaderModule ObjCreate::VKCreateShaderModule(VkShaderModuleCreateInfo* createInfo,
	VkAllocationCallbacks* allocator, VkShaderModule* shaderModule) {
	if (vkCreateShaderModule(*devices.logical, createInfo, allocator, shaderModule) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create shader module");
	}
	return *shaderModule;
}

VkSemaphore ObjCreate::VKCreateSemaphore(VkSemaphoreCreateInfo* semaphoreCreateInfo,
	VkAllocationCallbacks* allocator, VkSemaphore* semaphore) {
	if (vkCreateSemaphore(*devices.logical, semaphoreCreateInfo, allocator, semaphore) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create semaphore");
	}
	return *semaphore;
}

VkFence ObjCreate::VKCreateFence(VkFenceCreateInfo* fenceCreateInfo, VkAllocationCallbacks* allocator, VkFence* fence) {
	if (vkCreateFence(*devices.logical, fenceCreateInfo, allocator, fence) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create a fence!");
	}
	return *fence;
}

VkAccelerationStructureKHR ObjCreate::VKCreateAccelerationStructureKHR(
	VkAccelerationStructureCreateInfoKHR* accelerationStructureCreateInfo, VkAllocationCallbacks* allocator,
	VkAccelerationStructureKHR* accelerationStructureKHR) {
	//function pointer in CoreExtensions.hpp -- class instance 
	coreExtensions->vkCreateAccelerationStructureKHR(
		*devices.logical, accelerationStructureCreateInfo, allocator, accelerationStructureKHR);
	return *accelerationStructureKHR;
}
