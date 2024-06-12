#pragma once

#include <Utilities_EngCore.hpp>
#include <CoreExtensions.hpp>
#include <CoreDebug.hpp>

// -- object create
//@brief contains create functions for Vulkan Objects
class Utilities_CreateObject {
public:

	// -- devices struct
	//@brief contains references to logical and physical device
	struct Devices {
		// -- physical device reference
		VkPhysicalDevice* physical = nullptr;
		// -- logical device reference
		VkDevice* logical = nullptr;
	};

	// -- devices struct
	Devices devices;

	//debug ptr
	CoreDebug* pCoreDebug = nullptr;

	//core extensions ptr
	CoreExtensions* coreExtensions = nullptr;

	//command pool
	VkCommandPool commandPool{};

	// -- default constructor
	Utilities_CreateObject();

	// -- constructor
	//@brief calls initUtilities_CreateObject function and assigns values to devices struct containing references to devices
	Utilities_CreateObject(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice,
		CoreExtensions* coreExtensions, CoreDebug* coreDebug, VkCommandPool commandPool);

	// -- init function that is called in constructor
	void InitUtilities_CreateObject(VkPhysicalDevice* physicalDevice, VkDevice* logicalDevice,
		CoreExtensions* coreExtensions, CoreDebug* coreDebug, VkCommandPool commandPool);

	// -- create buffer
	// creates buffer passed in as parameter and returns it
	VkBuffer VKCreateBuffer(VkBufferCreateInfo* bufferCreateInfo, VkAllocationCallbacks* allocator, VkBuffer* buffer);

	// -- allocate memory
	//creates VkDeviceMemory object from parameter and returns it
	VkDeviceMemory VKAllocateMemory(VkMemoryAllocateInfo* memoryAllocateInfo, VkAllocationCallbacks* allocator,
		VkDeviceMemory* memory);

	// -- create command buffer
	//@brief create command buffer and begin if request
	//@brief does not end recording
	VkCommandBuffer VKCreateCommandBuffer(VkCommandBufferLevel level, bool begin);

	// -- create image
	//@brief creates image passed in parameter
	//@return image pass in parameter
	VkImage VKCreateImage(VkImageCreateInfo* imageCreateInfo, VkAllocationCallbacks* allocator, VkImage* image);

	// -- create image view
	//@brief creates image view passed in parameter
	//@return image view passed in parameter
	VkImageView VKCreateImageView(VkImageViewCreateInfo* createInfo, VkAllocationCallbacks* allocator, VkImageView* imageView);

	// -- create sampler
	//@brief creates image view passed in parameter
	//@return image view passed in parameter
	VkSampler VKCreateSampler(VkSamplerCreateInfo* createInfo, VkAllocationCallbacks* allocator, VkSampler* sampler);

	// -- create render pass
	//@brief creates render pass passed in parameter
	//@return render pass passed in parameter
	VkRenderPass VKCreateRenderPass(VkRenderPassCreateInfo* createInfo, VkAllocationCallbacks* allocator, VkRenderPass* renderPass);

	// -- create pipeline layout
	//@brief creates pipeline layout passed in parameter
	//@return pipeline layout passed in parameter
	VkPipelineLayout VKCreatePipelineLayout(VkPipelineLayoutCreateInfo* createInfo,
		VkAllocationCallbacks* allocator, VkPipelineLayout* pipelineLayout);

	// -- create raytracing pipeline
	//@brief creates raytracing pipeline passed in parameter
	//@return raytracing pipeline passed in parameter
	VkPipeline VKCreateRaytracingPipeline(VkRayTracingPipelineCreateInfoKHR* createInfo,
		VkAllocationCallbacks* allocator, VkPipeline* pipeline);

	// -- create graphics pipeline
	//@brief creates graphics pipeline passed in parameter
	//@return graphics pipeline passed in parameter 
	VkPipeline VKCreateGraphicsPipeline(VkPipelineCache pipelineCache, uint32_t createInfoCount, VkGraphicsPipelineCreateInfo* createInfos, VkAllocationCallbacks* allocator,
		VkPipeline* pipeline) {
		if (vkCreateGraphicsPipelines(*devices.logical, pipelineCache, createInfoCount, createInfos, allocator, pipeline) !=
			VK_SUCCESS) {
			throw std::invalid_argument("Failed to create UI pipeline!");
		}
		return *pipeline;
	}

	// -- create framebuffer
	//@brief creates framebuffer passed in parameter
	//@return framebuffer passed in parameter  
	VkFramebuffer VKCreateFramebuffer(VkFramebufferCreateInfo* framebufferCreateInfo, VkAllocationCallbacks* allocator, VkFramebuffer* framebuffer) {
		if (vkCreateFramebuffer(*devices.logical, framebufferCreateInfo, nullptr, framebuffer) != VK_SUCCESS) {
			throw std::invalid_argument("Failed to create framebuffer!");
		}
		return *framebuffer;
	}

	// -- create descriptor pool
	//@brief creates descriptor pool passed in parameter
	//@return descriptor pool passed in parameter
	VkDescriptorPool VKCreateDescriptorPool(VkDescriptorPoolCreateInfo* descriptorPoolCreateInfo,
		VkAllocationCallbacks* allocator, VkDescriptorPool* descriptorPool);

	// -- create descriptor set layout
	//@brief creates descriptor set layout passed in parameter
	//@return descriptor set layout passed in parameter
	VkDescriptorSetLayout VKCreateDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo* createInfo,
		VkAllocationCallbacks* allocator, VkDescriptorSetLayout* descriptorSetLayout);

	// -- create descriptor set
	//@brief creates descriptor set passed in parameter
	//@return descriptor set passed in parameter
	VkDescriptorSet VKAllocateDescriptorSet(VkDescriptorSetAllocateInfo* descriptorSetAllocateInfo,
		VkAllocationCallbacks* allocator, VkDescriptorSet* descriptorSet);

	// -- create shader module
	//@brief creates shader module passed in parameter
	//@return shader module passed in parameter
	VkShaderModule VKCreateShaderModule(VkShaderModuleCreateInfo* createInfo,
		VkAllocationCallbacks* allocator, VkShaderModule* shaderModule);

	// -- create semaphore
	//@brief creates semaphore passed in parameter
	//@return semaphore passed in parameter
	VkSemaphore VKCreateSemaphore(VkSemaphoreCreateInfo* semaphoreCreateInfo, VkAllocationCallbacks* allocator,
		VkSemaphore* semaphore);

	// -- create fence
	//@brief creates fence passed in parameter
	//@return fence passed in parameter
	VkFence VKCreateFence(VkFenceCreateInfo* fenceCreateInfo, VkAllocationCallbacks* allocator, VkFence* fence);

	// -- create acceleration structure
	//creates VkAccelerationStructureKHR from parameter and returns it
	VkAccelerationStructureKHR VKCreateAccelerationStructureKHR(VkAccelerationStructureCreateInfoKHR*
		accelerationStructureCreateInfo, VkAllocationCallbacks* allocator, VkAccelerationStructureKHR* accelerationStructureKHR);
};

