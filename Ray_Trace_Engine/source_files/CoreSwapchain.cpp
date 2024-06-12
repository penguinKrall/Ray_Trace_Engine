#include "CoreSwapchain.hpp"
#include <limits>

// -- ctor
CoreSwapchain::CoreSwapchain() {

}

// -- retrieve device swapchain data
void CoreSwapchain::retrieveDeviceSwapchainData(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {

	//get surface capabilities
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapchainData.surfaceCapabilities);

	//output surface capabilities
	vrt::Tools::outputSurfaceCapabilities(swapchainData.surfaceCapabilities);

	//formats
	uint32_t formatCount = 0;

	//get count
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,
		surface, &formatCount, nullptr);

	//make list
	if (formatCount != 0) {
		swapchainData.availableSurfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			physicalDevice, surface, &formatCount, swapchainData.availableSurfaceFormats.data());
	}

	//output available formats
	std::cout << "\nAvailable Surface Formats:\n";
	for (const auto& format : swapchainData.availableSurfaceFormats) {
		std::cout << "\tFormat: " << EnumStringHelper::vkFormatToString(format.format) <<
			", Color Space: " << EnumStringHelper::ColorSpaceToString(format.colorSpace) << "\n";
	}

	//assign surface format
	assignSwapchainImageFormat();

	//presentation modes
	//get count
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
		surface, &presentationCount, nullptr);

	//make list
	if (presentationCount != 0) {
		swapchainData.availablePresentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice,
			surface, &presentationCount, swapchainData.availablePresentationModes.data());
	}

	//output presentation modes
	std::cout << "\nAvailable Presentation Modes:\n";
	for (const auto& mode : swapchainData.availablePresentationModes) {
		std::string modeStr = EnumStringHelper::VkPresentModeToString(mode);
		std::cout << "\tMode: " << modeStr << "\n";
	}

	//assign present mode
	assignSwapchainPresentMode();

}

// -- assign swapchain image format
void CoreSwapchain::assignSwapchainImageFormat() {
	for (const auto& format : swapchainData.availableSurfaceFormats) {
		if (format.format == VK_FORMAT_R8G8B8A8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			swapchainData.assignedSwapchainImageFormat = format;
			std::cout << "\nAssigned Swapchain Image Format: " <<
				EnumStringHelper::vkFormatToString(swapchainData.assignedSwapchainImageFormat.format) <<
				"\nAssigned Swapchain Image Color Space: " <<
				EnumStringHelper::ColorSpaceToString(swapchainData.assignedSwapchainImageFormat.colorSpace) << std::endl;
			return;
		}
	}
}

// -- assign swapchain present mode
void CoreSwapchain::assignSwapchainPresentMode() {
	for (const auto& presentationMode : swapchainData.availablePresentationModes) {
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			swapchainData.assignedPresentMode = presentationMode;
			std::cout << "\nAssigned Present Mode: " <<
				EnumStringHelper::VkPresentModeToString(swapchainData.assignedPresentMode) << std::endl;
		}
	}
}

// -- retrieve surface extent
void CoreSwapchain::retrieveSurfaceExtent(GLFWwindow* window) {

	if (swapchainData.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {

		std::cout << std::endl << "Swapchain Surface Extent\n" <<
			" width: " << swapchainData.surfaceCapabilities.currentExtent.width <<
			"\n height: " << swapchainData.surfaceCapabilities.currentExtent.height << std::endl;

		swapchainData.swapchainExtent2D = swapchainData.surfaceCapabilities.currentExtent;
		//return surfaceCapabilities.currentExtent;
	}

	else {
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		newExtent.width = std::max(swapchainData.surfaceCapabilities.minImageExtent.width,
			std::min(swapchainData.surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(swapchainData.surfaceCapabilities.minImageExtent.height,
			std::min(swapchainData.surfaceCapabilities.maxImageExtent.height, newExtent.height));

		std::cout << "New Swapchain Extent" <<
			"\n width : " << newExtent.width <<
			"\n height : " << newExtent.height << std::endl;

		swapchainData.swapchainExtent2D = newExtent;
	}
}

// -- create swapchain images
std::vector<VkImage> CoreSwapchain::createSwapchainImage(VkDevice logicalDevice) {

	//resize
	uint32_t swapchainImageCount = frame_draws;
	if (swapchainData.swapchainImages.image.size() != frame_draws) {
		swapchainData.swapchainImages.image.resize(swapchainImageCount);
	}
	vkGetSwapchainImagesKHR(
		logicalDevice, swapchainData.swapchainKHR, &swapchainImageCount, nullptr);

	vkGetSwapchainImagesKHR(
		logicalDevice, swapchainData.swapchainKHR, &swapchainImageCount, swapchainData.swapchainImages.image.data());

	return swapchainData.swapchainImages.image;
}

// -- create swapchain image view
std::vector<VkImageView> CoreSwapchain::createSwapchainImageView(VkDevice logicalDevice) {

	//resize
	swapchainData.swapchainImages.imageView.resize(swapchainData.swapchainImages.image.size());

	//create info
	VkImageViewCreateInfo viewCreateInfo = {};

	//iterate
	for (int i = 0; i < frame_draws; i++) {

		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = swapchainData.swapchainImages.image[i];
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = swapchainData.assignedSwapchainImageFormat.format;
		viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;			// --------- miplevels ------------
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		//Create image view
		if (vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &swapchainData.swapchainImages.imageView[i]) != VK_SUCCESS) {
			throw std::invalid_argument("Failed to create Swapchain Image View!");
		}
	}

	//return imageView vector
	return swapchainData.swapchainImages.imageView;
}

// -- create swapchain
VkSwapchainKHR CoreSwapchain::createSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface,
	GLFWwindow* window, vrt::Tools::QueueFamilyIndices queueFamilyIndices) {

	//get swapchain data
	retrieveDeviceSwapchainData(physicalDevice, surface);

	//get extent
	retrieveSurfaceExtent(window);

	//Find how many images are in the swap chain. Get one more than the minimum to allow triple buffering.
	uint32_t imageCount = swapchainData.surfaceCapabilities.minImageCount + 1;

	//If image count higher than max, then clamp down to max.
	//If zero, then limitless.
	if (swapchainData.surfaceCapabilities.maxImageCount > 0 &&
		swapchainData.surfaceCapabilities.maxImageCount < imageCount) {
		imageCount = swapchainData.surfaceCapabilities.maxImageCount;
	}

	//output image count
	std::cout << "Swapchain Image Count: " << imageCount << std::endl;

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.imageFormat = swapchainData.assignedSwapchainImageFormat.format;
	swapchainCreateInfo.imageColorSpace = swapchainData.assignedSwapchainImageFormat.colorSpace;
	swapchainCreateInfo.presentMode = swapchainData.assignedPresentMode;
	swapchainCreateInfo.imageExtent = swapchainData.swapchainExtent2D;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchainCreateInfo.preTransform = swapchainData.surfaceCapabilities.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.clipped = false;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;

	std::cout << "graphics queue: " << queueFamilyIndices.graphics << "	present queue: " << queueFamilyIndices.present << std::endl;
	if (queueFamilyIndices.graphics != queueFamilyIndices.present) {
		std::array<uint32_t, 4> indices = {
		(uint32_t)queueFamilyIndices.graphics,
		(uint32_t)queueFamilyIndices.compute,
		(uint32_t)queueFamilyIndices.present,
		(uint32_t)queueFamilyIndices.transfer
		};

		swapchainCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
		swapchainCreateInfo.pQueueFamilyIndices = indices.data();
		std::cout << "[1]swapchainCreateInfo.queueFamilyIndexCount = " << swapchainCreateInfo.queueFamilyIndexCount << std::endl;
	}

	else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
		std::cout << "[2]swapchainCreateInfo.queueFamilyIndexCount = " << swapchainCreateInfo.queueFamilyIndexCount << std::endl;
	}

	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	//VkSwapchainKHR swapchain{};

	VkResult result = vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, nullptr, &swapchainData.swapchainKHR);
	if (result != VK_SUCCESS) {
		throw std::invalid_argument("Failed to create a Swapchain!");
	}

	return swapchainData.swapchainKHR;

}

// -- destroy swapchain class
void CoreSwapchain::DestroySwapchain(VkDevice logicalDevice) {

	//destroy image views
	for (const auto& swapchainimageviews : swapchainData.swapchainImages.imageView) {
		vkDestroyImageView(logicalDevice, swapchainimageviews, nullptr);
	}

	//destroy swapchain
	vkDestroySwapchainKHR(logicalDevice, swapchainData.swapchainKHR, nullptr);

}
