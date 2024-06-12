#pragma once

#include <Tools.hpp>

class CoreSwapchain {
public:

	//swapchain data

	struct SwapchainImage {
		std::vector<VkImage> image{};
		std::vector <VkImageView> imageView{};
		std::vector <VkDeviceMemory> imageMemory{};
	};

	struct SwapchainData {
		VkSurfaceCapabilitiesKHR surfaceCapabilities;
		std::vector<VkSurfaceFormatKHR> availableSurfaceFormats;
		std::vector<VkPresentModeKHR> availablePresentationModes;
		VkPresentModeKHR assignedPresentMode{};
		VkSurfaceFormatKHR assignedSwapchainImageFormat;
		VkSwapchainKHR swapchainKHR{};
		VkExtent2D swapchainExtent2D{};
		SwapchainImage swapchainImages;
	};


	//structs
	SwapchainData swapchainData{};

	
	//ctor
	CoreSwapchain();

	//funcs
	void retrieveDeviceSwapchainData(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	void assignSwapchainImageFormat();
	void assignSwapchainPresentMode();
	void retrieveSurfaceExtent(GLFWwindow* window);
	//need to create "create swapchain images" function
	std::vector<VkImage> createSwapchainImage(VkDevice logicalDevice);
	std::vector<VkImageView> createSwapchainImageView(VkDevice logicalDevice);
	VkSwapchainKHR createSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface,
		GLFWwindow* window, vrt::Tools::QueueFamilyIndices queueFamilyIndices);

	void DestroySwapchain(VkDevice logicalDevice);
};

