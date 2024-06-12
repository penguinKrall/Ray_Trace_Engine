#include "Commands.hpp"

Commands::Commands() {

}

VkCommandPool Commands::createCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex) {
	//create info
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	std::cout << "Creating Graphics Command Pool\n Queue Family Index: " << queueFamilyIndex << std::endl;
	//create
	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &this->commandPools.graphics) != VK_SUCCESS) {
		throw std::invalid_argument("Failed to create graphics command pool!");
	}

	return commandPools.graphics;
}

VkCommandPool Commands::CreateComputeCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex) {
	//create info
	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndex;

	std::cout << "Creating COMPUTE Command Pool\n Queue Family Index: " << queueFamilyIndex << std::endl;
	//create
	if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &this->commandPools.compute) != VK_SUCCESS) {
		throw std::invalid_argument("Failed to create compute command pool!");
	}

	return commandPools.compute;
}

VkQueue Commands::VKCreateQueue(VkDevice logicalDevice, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* queue) {
	vkGetDeviceQueue(logicalDevice, queueFamilyIndex, queueIndex, queue);
	return *queue;
}

std::vector<VkCommandBuffer> Commands::createGraphicsCommandBuffers(VkDevice logicalDevice, VkPhysicalDevice physicalDevice) {

	this->commandBuffers.graphics.resize(frame_draws);

	VkCommandBufferAllocateInfo cbAllocInfo = {};
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = commandPools.graphics;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(this->commandBuffers.graphics.size());

	if (vkAllocateCommandBuffers(logicalDevice, &cbAllocInfo, this->commandBuffers.graphics.data()) != VK_SUCCESS) {
		throw std::invalid_argument("Failed to allocate core Graphics Command Buffers!");
	}

	return this->commandBuffers.graphics;

}

std::vector<VkSemaphore> Commands::CreatePresentSemaphore(VkDevice logicalDevice) {

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	this->sync.presentFinishedSemaphore.resize(frame_draws);

	for (int i = 0; i < frame_draws; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &sync.presentFinishedSemaphore[i]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create imageAvailable semaphore");
		}
	}

	return sync.presentFinishedSemaphore;
}

std::vector<VkSemaphore> Commands::CreateRenderSemaphore(VkDevice logicalDevice) {

	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	this->sync.renderFinishedSemaphore.resize(frame_draws);

	for (int i = 0; i < frame_draws; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &sync.renderFinishedSemaphore[i]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create renderFinished semaphore");
		}
	}
	return sync.renderFinishedSemaphore;

}

std::vector<VkSemaphore> Commands::CreateComputeSemaphore(VkDevice logicalDevice) {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	this->sync.computeFinishedSemaphore.resize(frame_draws);

	for (int i = 0; i < frame_draws; i++) {
		if (vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr, &sync.computeFinishedSemaphore[i]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create Compute Finished semaphore");
		}
	}
	return sync.computeFinishedSemaphore;
}

std::vector<VkFence> Commands::CreateFences(VkDevice logicalDevice) {

	//fences (Used to check draw command buffer completion)
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	//create in signaled state so we don't wait on first render of each command buffer
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//resize
	sync.drawFences.resize(frame_draws);

	//create fences
	for (auto& fence : sync.drawFences) {
		if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create a fence!");
		}
	}

	return sync.drawFences;

}

std::vector<VkFence> Commands::CreateComputeFences(VkDevice logicalDevice) {

	//fences (Used to check draw command buffer completion)
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	//create in signaled state so we don't wait on first render of each command buffer
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	//resize
	sync.computeFences.resize(frame_draws);

	//create fences
	for (auto& fence : sync.computeFences) {
		if (vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fence) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create a compute fence!");
		}
	}

	return sync.computeFences;

}

void Commands::DestroyCommands(VkDevice logicalDevice) {

	//--sync
	//fences
	for (const auto& fence : sync.drawFences) {
		vkDestroyFence(logicalDevice, fence, nullptr);
	}

	for (const auto& fence : sync.computeFences) {
		vkDestroyFence(logicalDevice, fence, nullptr);
	}

	//semaphores
	for (int i = 0; i < frame_draws; i++) {
	vkDestroySemaphore(logicalDevice, sync.renderFinishedSemaphore[i], nullptr);
	vkDestroySemaphore(logicalDevice, sync.presentFinishedSemaphore[i], nullptr);
	vkDestroySemaphore(logicalDevice, sync.computeFinishedSemaphore[i], nullptr);
	}

	//--command pool
	vkDestroyCommandPool(logicalDevice, commandPools.graphics, nullptr);
	vkDestroyCommandPool(logicalDevice, commandPools.compute, nullptr);
}
