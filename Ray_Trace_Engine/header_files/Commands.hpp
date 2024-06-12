#pragma once

#include <Tools.hpp>

// -- Commands 
class Commands {
public:

	// -- current frame
	uint32_t currentFrame = 0;

	struct Queue {
		//Queue family properties of the physical device
		std::vector<VkQueueFamilyProperties> queueFamilyProperties;
		vrt::Tools::QueueFamilyIndices queueFamilyIndices{};
		VkQueue graphics{};
		VkQueue compute{};
		VkQueue present{};
		VkQueue transfer{};
		//std::set<int> indices;
	};

	//synchronization data
	struct Synchronization {
		std::vector<VkSemaphore> presentFinishedSemaphore;
		std::vector<VkSemaphore> renderFinishedSemaphore;
		std::vector<VkSemaphore> computeFinishedSemaphore;
		std::vector<VkFence> drawFences;
		std::vector<VkFence> computeFences;
		//int currentFrame = 0;
	};

	//command pools
	struct CommandPools {
		VkCommandPool graphics = VK_NULL_HANDLE;
		VkCommandPool compute = VK_NULL_HANDLE;
	};

	//command buffers
	struct CommandBuffers {
		std::vector<VkCommandBuffer> graphics{};
	};

	//--structs
	//queue
	Queue queue{};
	//sync
	Synchronization sync{};
	//command pool
	CommandPools commandPools{};
	//command buffers
	CommandBuffers commandBuffers{};

	// -- ctor
	Commands();

	// -- command pool
	VkCommandPool createCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex);
	VkCommandPool CreateComputeCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex);
	VkQueue VKCreateQueue(VkDevice logicalDevice, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* queue);

	// -- command buffers
	std::vector<VkCommandBuffer> createGraphicsCommandBuffers(VkDevice logicalDevice,
		VkPhysicalDevice physicalDevice);


	std::vector<VkSemaphore> CreatePresentSemaphore(VkDevice logicalDevice);

	std::vector<VkSemaphore> CreateRenderSemaphore(VkDevice logicalDevice);

	std::vector<VkSemaphore> CreateComputeSemaphore(VkDevice logicalDevice);


	std::vector<VkFence> CreateFences(VkDevice logicalDevice);

	std::vector<VkFence> CreateComputeFences(VkDevice logicalDevice);

	//void prepareSynchronizationPrimitives(VkDevice logicalDevice) {

	//	// Semaphores (Used for correct command ordering)
	//	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	//	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	//	semaphoreCreateInfo.pNext = nullptr;

	//	// Semaphore used to ensure that image presentation is complete before starting to submit again
	//	if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &sync.presentFinishedSemaphore) != VK_SUCCESS) {
	//		throw std::invalid_argument("failed to create imageAvailable semaphore");
	//	}

	//	// Semaphore used to ensure that all commands submitted have been finished before submitting the image to the queue
	//	if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &sync.renderFinishedSemaphore) != VK_SUCCESS) {
	//		throw std::invalid_argument("failed to create renderFinished semaphore");
	//	}

	//	// Fences (Used to check draw command buffer completion)
	//	VkFenceCreateInfo fenceCreateInfo = {};
	//	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//	// Create in signaled state so we don't wait on first render of each command buffer
	//	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	//	sync.drawFences.resize(frame_draws);
	//	for (auto& fence : sync.drawFences) {
	//		if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
	//			throw std::invalid_argument("failed to create a fence!");
	//		}
	//	}
	//}

	// -- destroy
	void DestroyCommands(VkDevice logicalDevice);
};

