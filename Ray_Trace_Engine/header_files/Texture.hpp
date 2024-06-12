#pragma once

#include <Tools.hpp>
#include <CoreBase.hpp>

#include <iostream>
#include <filesystem>

#include <ktx.h>
#include <ktxvulkan.h>

#include <tiny_gltf.h>

namespace vrt {

	// - texture class
	//@brief
	class Texture {

	public:

		VkImage image;
		VkImageLayout imageLayout;
		VkDeviceMemory imageMemory;
		VkImageView view;
		uint32_t width, height;
		uint32_t mipLevels;
		uint32_t layerCount;
		VkDescriptorImageInfo descriptor;
		VkSampler sampler;

		uint32_t index = -1;

		// -- core pointer
		CoreBase* pCoreBase = nullptr;

		// -- default ctor
		Texture();

		// -- init ctor
		Texture(CoreBase* coreBase);

		// -- init func
		void InitTexture(CoreBase* coreBase);

		// -- update descriptor
		void updateDescriptor();

		ktxResult loadKTXFile(std::string filename, ktxTexture** target);

		// -- load from file
		//@brief test func to make sure loading ktx files is possible
		void loadFromFile(
			std::string        filename,
			VkFormat           format,
			VkImageUsageFlags  imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
			VkImageLayout      imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			bool               forceLinear = false);

		void DestroyTexture();

		void fromglTfImage(tinygltf::Image& gltfimage, std::string path, CoreBase* coreBase, VkQueue copyQueue);



	};

}
